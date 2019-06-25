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
#include <toolkits/feature_engineering/feature_binner.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::sdk_model::feature_engineering;

/**
 * Generate the data from the model.
 */
gl_sframe generate_data(std::map<std::string, flexible_type> opts) {

  // Create a random SFrame.
  size_t examples = opts.at("examples");
  std::vector<std::string> feature_names;
  std::vector<flex_type_enum> feature_types;
  feature_names = {"ints", "reals"};
  feature_types = {flex_type_enum::INTEGER, 
                   flex_type_enum::FLOAT};

  gl_sframe_writer writer(feature_names, feature_types, 1);
  for(size_t i=0; i < examples; i++){
    std::vector<flexible_type> row;

    // Integer, float, and string columns
    row.push_back(3 * std::pow(10, i));
    row.push_back(5.0 * i);

    // Write row
    writer.write(row, 0);
  }
  return writer.close();
}


/**
 * Construct a model from data and options.
 */
std::shared_ptr<feature_binner> init_model(gl_sframe data, 
                       std::map<std::string, flexible_type> opts) {

  std::shared_ptr<feature_binner> model;
  model.reset(new feature_binner);
  std::map<std::string, flexible_type> options;
  for (const auto& kvp: opts){
    if (kvp.first != "examples" && kvp.first != "features") {
      options[kvp.first] = kvp.second;
    }
  }
  options["exclude"] = false;
  options["features"] = FLEX_UNDEFINED;
  model->init_transformer(options);
  model->fit(data);
  return model;

}

/**
 * Save the model and load it back.
 */
std::shared_ptr<feature_binner> save_and_load_model(
                                std::shared_ptr<feature_binner> model) {

  std::shared_ptr<feature_binner> loaded_model;
  loaded_model.reset(new feature_binner);

  dir_archive archive_write;
  archive_write.open_directory_for_write("feature_binner_tests");
  turi::oarchive oarc(archive_write);
  oarc << *model;
  archive_write.close();

  dir_archive archive_read;
  archive_read.open_directory_for_read("feature_binner_tests");
  turi::iarchive iarc(archive_read);
  iarc >> *loaded_model;
  return loaded_model;
}

/**
 * Save the model and load it back.
 */
void check_model(std::shared_ptr<feature_binner> model,
                 gl_sframe data,
                 std::map<std::string, flexible_type> opts) {

  std::map<std::string, flexible_type> _options;
  std::vector<std::string> _list_fields;
  flexible_type _get;

  // Answers.
  std::map<std::string, flexible_type> default_options = { 
    {"exclude", false}
  };
  std::vector<std::string> _list_fields_ans = {
        "excluded_features",
        "features",
        "bins"
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
  auto observed_cols = variant_get_value<std::vector<std::string>>(
                            model->get_value_from_state("features"));
  TS_ASSERT(data.column_names() == observed_cols);
  
  // Check options
  _options = model->get_current_options();
  for (auto& kvp: options){
    TS_ASSERT_EQUALS(_options[kvp.first],  kvp.second);
  }

  // Default options
  _options = model->get_default_options();
  for (auto& kvp: default_options){
    TS_ASSERT_EQUALS(_options[kvp.first],  kvp.second);
  }

  // Check list_fields
  _list_fields = model->list_fields();
  for(const auto& f: _list_fields_ans){
    TS_ASSERT(std::find(_list_fields.begin(), _list_fields.end(), f) 
                                                    != _list_fields.end());
  }

  // Check that transformations don't die.
  // ----------------------------------------------------------------------
  gl_sframe out_sf = model->transform(data);
  TS_ASSERT(out_sf.size() == data.size());

  // Uncomment for printing
  ((out_sf.get_proxy())->get_underlying_sframe())->debug_print();

  // Uncomment for printing
  // gl_sframe bin_sf = variant_get_value<gl_sframe>(model->get_value_from_state("bins"));
  // ((bin_sf.get_proxy())->get_underlying_sframe())->debug_print();

}

void run_feature_binner_test(std::map<std::string, flexible_type> opts) {

  gl_sframe data = generate_data(opts);
  logprogress_stream << std::endl;
  ((data.get_proxy())->get_underlying_sframe())->debug_print();

  std::shared_ptr<feature_binner> model = init_model(data, opts);
  check_model(model, data, opts);

  std::shared_ptr<feature_binner> loaded_model = save_and_load_model(model);
  check_model(loaded_model, data, opts);
}


/**
 *  Run tests.
*/
struct feature_binner_test  {
 
  public:


  void test_feature_binner_basic_log_scale() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 8}, 
      {"strategy", "logarithmic"}}; 
    run_feature_binner_test(opts);
  }
 
   void test_feature_binner_basic_quantiles() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 8}, 
      {"strategy", "quantile"}}; 
    run_feature_binner_test(opts);
  }
 
};

BOOST_FIXTURE_TEST_SUITE(_feature_binner_test, feature_binner_test)
BOOST_AUTO_TEST_CASE(test_feature_binner_basic_log_scale) {
  feature_binner_test::test_feature_binner_basic_log_scale();
}
BOOST_AUTO_TEST_CASE(test_feature_binner_basic_quantiles) {
  feature_binner_test::test_feature_binner_basic_quantiles();
}
BOOST_AUTO_TEST_SUITE_END()
