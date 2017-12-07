#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <numerics/armadillo.hpp>

#include <unity/lib/variant.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/dml/dml_class_registry.hpp>
#include <toolkits/supervised_learning/boosted_trees.hpp>
#include <toolkits/supervised_learning/random_forest.hpp>
#include <sframe/testing_utils.hpp>

#include <unity/dml/dml_toolkit_runner.hpp>

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
using namespace turi;
using namespace turi::supervised;
using namespace turi::supervised::xgboost;

/**
 * Test suite for distributed xgboost models.
*/
struct distributed_xgboost_test {

 public:
  void test_boosted_trees_regression() {
    std::map<std::string, flexible_type> opts = {
      {"model_name", "boosted_trees_regression"},
      {"examples", 1000}, 
      {"features", 10}}; 
    size_t n = runner.get_default_num_workers_from_env(); 
    test_impl(opts, n);
  }

  void test_random_forest_regression() {
    std::map<std::string, flexible_type> opts = {
      {"model_name", "random_forest_regression"},
      {"examples", 1000}, 
      {"features", 10}}; 
    size_t n = runner.get_default_num_workers_from_env(); 
    test_impl(opts, n);
  }

  void test_boosted_trees_classifier() {
    std::map<std::string, flexible_type> opts = {
      {"model_name", "boosted_trees_classifier"},
      {"examples", 1000}, 
      {"features", 10}}; 
    size_t n = runner.get_default_num_workers_from_env(); 
    test_impl(opts, n);
  }

  void test_random_forest_classifier() {
    std::map<std::string, flexible_type> opts = {
      {"model_name", "random_forest_classifier"},
      {"examples", 1000}, 
      {"features", 10}}; 
    size_t n = runner.get_default_num_workers_from_env(); 
    test_impl(opts, n);
  }

  void setup() {
    runner.set_library("libdistributed_supervised_learning.so");
    dml_class_registry::get_instance().register_model<boosted_trees_regression>();
    dml_class_registry::get_instance().register_model<boosted_trees_classifier>();
    dml_class_registry::get_instance().register_model<random_forest_regression>();
    dml_class_registry::get_instance().register_model<random_forest_classifier>();
    working_dir = turi::get_temp_name();
    fileio::create_directory(working_dir);
  }

  void teardown() {
    fileio::delete_path_recursive(working_dir);
  }

  void test_impl(std::map<std::string, flexible_type> opts,
                 size_t num_workers) {

    setup();

    try {
      // Arrange
      // ----------------------------------------------------------------------
      size_t examples = opts.at("examples");
      size_t features = opts.at("features");
      std::string model_name = opts.at("model_name");

      // Feature names
      std::string feature_types;
      for(size_t i=0; i < features; i++){
        feature_types += "n";
      }

      // Generate some data.
      sframe data = make_random_sframe(examples, feature_types, true);
      sframe y = data.select_columns({"target"});
      sframe X = data;
      X = X.remove_column(X.column_index("target"));

      std::shared_ptr<unity_sframe> uX(new unity_sframe());
      std::shared_ptr<unity_sframe> uy(new unity_sframe());
      uX->construct_from_sframe(X);
      uy->construct_from_sframe(y);
      gl_sframe gl_X = uX;
      gl_sframe gl_y = uy;
      gl_y["target"] = gl_y["target"] > gl_y["target"].mean();

      // Setup the arguments.
      std::map<std::string, flexible_type> options = { };
      variant_map_type params;
      params["features"] = to_variant(gl_X);
      params["target"] = to_variant(gl_y);
      params["model_name"] = model_name;
      for (const auto& kvp: options){
        params[kvp.first] = to_variant(kvp.second);
      }

      // Act
      // ----------------------------------------------------------------------
      // Train the model.
      variant_type ret = runner.run("distributed_supervised_train", params, working_dir, num_workers);
      std::shared_ptr<xgboost_model> model =
            variant_get_value<std::shared_ptr<xgboost_model>>(ret);

      // Assert
      // ----------------------------------------------------------------------
      // Check options.
      TS_ASSERT(model->name() == model_name);
      std::map<std::string, flexible_type> _options = model->get_current_options();
      for (auto& kvp: options){
        TS_ASSERT(_options[kvp.first] == kvp.second);
      }
      TS_ASSERT(model->is_trained() == true);
    } catch (...) {
      teardown();
      throw;
    }
    teardown();
  }

  dml_toolkit_runner runner;
  std::string working_dir;
};

BOOST_FIXTURE_TEST_SUITE(_distributed_xgboost_test, distributed_xgboost_test)
BOOST_AUTO_TEST_CASE(test_boosted_trees_regression) {
  distributed_xgboost_test::test_boosted_trees_regression();
}
BOOST_AUTO_TEST_CASE(test_random_forest_regression) {
  distributed_xgboost_test::test_random_forest_regression();
}
BOOST_AUTO_TEST_CASE(test_boosted_trees_classifier) {
  distributed_xgboost_test::test_boosted_trees_classifier();
}
BOOST_AUTO_TEST_CASE(test_random_forest_classifier) {
  distributed_xgboost_test::test_random_forest_classifier();
}
BOOST_AUTO_TEST_SUITE_END()
