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
#include <toolkits/feature_engineering/sample_transformer.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::sdk_model::feature_engineering;

/**
 * Generate the data from the model.
 */
gl_sframe generate_data(std::map<std::string, flexible_type> opts) {

  // Create a random SFrame.
  size_t features = opts.at("features");
  size_t examples = opts.at("features");
  std::vector<std::string> feature_names;
  std::vector<flex_type_enum> feature_types;
  for (size_t i = 0; i < features; i++){
    feature_names.push_back(std::to_string(i));
    feature_types.push_back(flex_type_enum::FLOAT);
  }

  gl_sframe_writer writer(feature_names, feature_types, 1);
  for(size_t i=0; i < examples; i++){
    std::vector<flexible_type> row (features, 0);
    writer.write(row, 0);
  }
  return writer.close();
}


/**
 * Construct a model from data and options.
 */
std::shared_ptr<sample_transformer> init_model(gl_sframe data, 
                       std::map<std::string, flexible_type> opts) {

  std::shared_ptr<sample_transformer> model;
  model.reset(new sample_transformer);
  
  // Init and train
  std::map<std::string, flexible_type> kwargs;
  kwargs["constant"] = opts["constant"];
  kwargs["features"] = variant_get_value<flexible_type>(to_variant(data.column_names()));

  model->init_transformer(kwargs);
  model->fit(data);
  return model;

}

/**
 * Save the model and load it back.
 */
std::shared_ptr<sample_transformer> save_and_load_model(
                                std::shared_ptr<sample_transformer> model) {

  std::shared_ptr<sample_transformer> loaded_model;
  loaded_model.reset(new sample_transformer);

  dir_archive archive_write;
  archive_write.open_directory_for_write("sample_transformer_tests");
  turi::oarchive oarc(archive_write);
  oarc << *model;
  archive_write.close();

  dir_archive archive_read;
  archive_read.open_directory_for_read("sample_transformer_tests");
  turi::iarchive iarc(archive_read);
  iarc >> *loaded_model;
  return loaded_model;

}

/**
 * Save the model and load it back.
 */
void check_model(std::shared_ptr<sample_transformer> model,
                 gl_sframe data,
                 std::map<std::string, flexible_type> opts) {

  std::map<std::string, flexible_type> _options;
  std::vector<std::string> _list_fields;
  flexible_type _get;

  // Answers.
  size_t constant = opts.at("constant");
  std::map<std::string, flexible_type> options = { 
    {"constant", constant}
  };
  std::map<std::string, flexible_type> default_options = { 
    {"constant", 0.5}
  };
  std::vector<std::string> _list_fields_ans = {
        "constant",
        "features",
	    "num_features",
  };


  // Check the model
  // ----------------------------------------------------------------------
  TS_ASSERT(model->get_constant() == options["constant"]);
  TS_ASSERT_EQUALS(data.num_columns(), variant_get_value<double>(model->get_value_from_state("num_features")));
  TS_ASSERT(data.column_names() == variant_get_value<std::vector<std::string>>(
                            model->get_value_from_state("features")));

  // Check options
  _options = model->get_current_options();
  for (auto& kvp: options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }

  // Default options
  _options = model->get_default_options();
  for (auto& kvp: default_options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }

  // Check list_fields
  _list_fields = model->list_fields();
  for(const auto& f: _list_fields_ans){
    TS_ASSERT(std::find(_list_fields.begin(), _list_fields.end(), f) 
                                                    != _list_fields.end());
  }

  // Check transformations.
  // ----------------------------------------------------------------------
  gl_sframe pred = model->transform(data);
  for(const std::string& f: data.column_names()){
    TS_ASSERT( (pred[f] - options["constant"]).sum() < 1e-5);
  }

}

void run_sample_transformer_test(std::map<std::string, flexible_type> opts) {

  gl_sframe data = generate_data(opts);
  std::shared_ptr<sample_transformer> model = init_model(data, opts);
  std::shared_ptr<sample_transformer> loaded_model = save_and_load_model(model);
  check_model(model, data, opts);
  check_model(loaded_model, data, opts);
}


/**
 *  Check linear supervised 
*/
struct sample_transformer_test  {
 
  public:

  void test_sample_transformer_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 100}, 
      {"constant", 1}, 
      {"features", 1}}; 
    run_sample_transformer_test(opts);
  }
  
  void test_sample_transformer_small() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 1000}, 
      {"constant", 0}, 
      {"features", 10}}; 
    run_sample_transformer_test(opts);
  }
  
};

BOOST_FIXTURE_TEST_SUITE(_sample_transformer_test, sample_transformer_test)
BOOST_AUTO_TEST_CASE(test_sample_transformer_basic_2d) {
  sample_transformer_test::test_sample_transformer_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_sample_transformer_small) {
  sample_transformer_test::test_sample_transformer_small();
}
BOOST_AUTO_TEST_SUITE_END()
