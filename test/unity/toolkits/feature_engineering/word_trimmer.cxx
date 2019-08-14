#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <stdlib.h>
#include <vector>
#include <string>
#include <functional>
#include <random>

#include <core/data/sframe/gl_sframe.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/feature_engineering/word_trimmer.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::sdk_model::feature_engineering;

/**
 * Generate the data from the model.
 */
gl_sframe generate_data(std::map<std::string, flexible_type>& opts) {

  // Create a random SFrame with three columns: string, dict, and list.
  size_t examples = opts.at("examples");
  std::vector<std::string> feature_names = {"string", "dict", "list"};
  std::vector<flex_type_enum> feature_types = {
    flex_type_enum::STRING, 
    flex_type_enum::DICT, 
    flex_type_enum::LIST};

  gl_sframe_writer writer(feature_names, feature_types, 1);
  for(size_t i=0; i < examples; i++){
    std::vector<flexible_type> row;

    // string column
    row.push_back("this is sentence " + std::to_string(i) + " it is it is!,,!");

    // dict column
    flex_dict temp = {};
    temp.push_back({"puppy", 2 * i});
    temp.push_back({"cat", i});
    row.push_back(temp);

    // list column
    flex_list temp_list = {"this", "is", "sentence", std::to_string(i)};
    row.push_back(temp_list);

    // output this row
    writer.write(row, 0);
  }
  return writer.close();
}

/**
 * Generate bad dictionaries with string values to test for errors.
 */
gl_sframe generate_bad_dict(std::map<std::string, flexible_type>& opts) {

  // Create a random SFrame
  size_t examples = opts.at("examples");
  std::vector<std::string> feature_names = {"dict"};
  std::vector<flex_type_enum> feature_types = {flex_type_enum::DICT};

  gl_sframe_writer writer(feature_names, feature_types, 1);
  for(size_t i=0; i < examples; i++){
    std::vector<flexible_type> row;

    flex_dict temp = {};
    temp.push_back({"puppy", std::to_string(2*i)});
    temp.push_back({"cat", i});
    row.push_back(temp);

    // output this row
    writer.write(row, 0);
  }
  return writer.close();
}

/**
 * Generate bad lists with non-string values to test for errors.
 */
gl_sframe generate_bad_list(std::map<std::string, flexible_type>& opts) {

  // Create a random SFrame
  size_t examples = opts.at("examples");
  std::vector<std::string> feature_names = {"list"};
  std::vector<flex_type_enum> feature_types = {flex_type_enum::LIST};

  gl_sframe_writer writer(feature_names, feature_types, 1);
  for(size_t i=0; i < examples; i++){
    std::vector<flexible_type> row;

    // list column
    flex_list temp_list = {"sentence", i};
    row.push_back(temp_list);

    // output this row
    writer.write(row, 0);
  }
  return writer.close();
}

/**
 * Construct a model from data and options.
 */
std::shared_ptr<word_trimmer> init_model(const gl_sframe& data, 
                       std::map<std::string, flexible_type>& opts) {

  std::shared_ptr<word_trimmer> model;
  model.reset(new word_trimmer);
  std::map<std::string, flexible_type> options;
  for (const auto& kvp: opts){
    if (kvp.first != "examples" && kvp.first != "features") {
      options[kvp.first] = kvp.second;
    }
  }
  options["features"] = FLEX_UNDEFINED;
  model->init_transformer(options);
  model->fit(data);
  return model;

}

/**
 * Save the model and load it back.
 */
std::shared_ptr<word_trimmer> save_and_load_model(
                                std::shared_ptr<word_trimmer> model) {

  std::shared_ptr<word_trimmer> loaded_model;
  loaded_model.reset(new word_trimmer);

  dir_archive archive_write;
  archive_write.open_directory_for_write("word_trimmer_tests");
  turi::oarchive oarc(archive_write);
  oarc << *model;
  archive_write.close();

  dir_archive archive_read;
  archive_read.open_directory_for_read("word_trimmer_tests");
  turi::iarchive iarc(archive_read);
  iarc >> *loaded_model;
  return loaded_model;
}

/**
 * Save the model and load it back.
 */
void check_model(std::shared_ptr<word_trimmer> model,
                 gl_sframe& data,
                 std::map<std::string, flexible_type>& opts) {

  std::map<std::string, flexible_type> _options;
  std::vector<std::string> _list_fields;
  flexible_type _get;

  // Answers.
  std::map<std::string, flexible_type> default_options = { 
    {"to_lower", true},
    {"output_column_prefix", flex_undefined()},
    {"stopwords", flex_undefined()},
    {"threshold", 2},
    {"delimiters", flex_list({"\r", "\v", "\n", "\f", "\t", " "})}
  };
  
  std::vector<std::string> _list_fields_ans = {
        "features",
        "excluded_features",
        "to_lower",
        "output_column_prefix",
        "threshold",
        "stopwords",
        "vocabulary",
        "delimiters"
  };
  std::map<std::string, flexible_type> options;
  for (const auto& kvp: default_options){
    if (opts.count(kvp.first) == 0) { 
      options[kvp.first] = kvp.second;
    } else {
      options[kvp.first] = opts[kvp.first];
    }
  }

  // Check the model
  // ----------------------------------------------------------------------
  TS_ASSERT(data.column_names() == variant_get_value<std::vector<std::string>>(
                            model->get_value_from_state("features")));

  // Check options
  _options = model->get_current_options();
  for (auto& kvp: options){
    TS_ASSERT_EQUALS(_options[kvp.first],  kvp.second);
  }
  TS_ASSERT_EQUALS(_options.size(), options.size());

  // Default options
  _options = model->get_default_options();
  for (auto& kvp: default_options){
    TS_ASSERT_EQUALS(_options[kvp.first],  kvp.second);
  }
  TS_ASSERT_EQUALS(_options.size(), default_options.size());

  // Check list_fields
  _list_fields = model->list_fields();
  for(const auto& f: _list_fields_ans){
    TS_ASSERT(std::find(_list_fields.begin(), _list_fields.end(), f) 
                                                    != _list_fields.end());
  }
  TS_ASSERT_EQUALS(_list_fields.size(), _list_fields_ans.size());

  // Check that transformations don't die.
  // ----------------------------------------------------------------------
  gl_sframe out_sf = model->transform(data);
  TS_ASSERT(out_sf.size() == data.size());

  // Uncomment for printing
  //((out_sf.get_proxy())->get_underlying_sframe())->debug_print();
}

void run_word_trimmer_test(std::map<std::string, flexible_type>& opts) {

  gl_sframe data = generate_data(opts);
  std::shared_ptr<word_trimmer> model = init_model(data, opts);
  std::shared_ptr<word_trimmer> loaded_model = save_and_load_model(model);
  check_model(model, data, opts);
  check_model(loaded_model, data, opts);
}

void run_bad_input_dict_test(std::map<std::string, flexible_type>& opts) {

  gl_sframe data = generate_bad_dict(opts);

  TS_ASSERT_THROWS_ANYTHING(
    std::shared_ptr<word_trimmer> model = init_model(data, opts);
    model->transform(data);
  );
}

void run_bad_input_list_test(std::map<std::string, flexible_type>& opts) {

  gl_sframe data = generate_bad_list(opts);

  TS_ASSERT_THROWS_ANYTHING(
    std::shared_ptr<word_trimmer> model = init_model(data, opts);
    model->transform(data);
  );
}


/**
 *  Run tests.
*/
struct word_trimmer_test  {
 
  public:

  void test_word_trimmer_basic() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 20}, 
      {"to_lower", true},
      {"exclude", false},
      {"delimiters", flex_undefined()}}; 
    run_word_trimmer_test(opts);
  }

  void test_bad_input_dict() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 20}, 
      {"to_lower", true},
      {"exclude", false},
      {"delimiters", flex_undefined()}}; 
    run_bad_input_dict_test(opts);
  }

  void test_bad_input_list() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 20}, 
      {"to_lower", true},
      {"exclude", false},
      {"delimiters", flex_undefined()}}; 
    run_bad_input_list_test(opts);
  }
};

BOOST_FIXTURE_TEST_SUITE(_word_trimmer_test, word_trimmer_test)
BOOST_AUTO_TEST_CASE(test_word_trimmer_basic) {
  word_trimmer_test::test_word_trimmer_basic();
}
BOOST_AUTO_TEST_CASE(test_bad_input_dict) {
  word_trimmer_test::test_bad_input_dict();
}
BOOST_AUTO_TEST_CASE(test_bad_input_list) {
  word_trimmer_test::test_bad_input_list();
}
BOOST_AUTO_TEST_SUITE_END()
