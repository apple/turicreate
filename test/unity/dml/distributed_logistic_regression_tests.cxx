#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <numerics/armadillo.hpp>

#include <unity/lib/variant.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <toolkits/supervised_learning/logistic_regression.hpp>
#include <unity/dml/dml_class_registry.hpp>
#include <sframe/testing_utils.hpp>

#include <unity/dml/dml_toolkit_runner.hpp>

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
using namespace turi;
using namespace turi::supervised;

/**
 * Test suite for distributed logistic regression. 
*/
struct distributed_logistic_regression_test  {

 public:
  void test_logistic_regression_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 100}, 
      {"features", 1}}; 

    size_t n = runner.get_default_num_workers_from_env(); 
    test_impl(opts, n);
  }
  
  void test_logistic_regression_small() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 1000}, 
      {"features", 10}}; 

    size_t n = runner.get_default_num_workers_from_env(); 
    test_impl(opts, n);
  }


  void setup() {
    runner.set_library("libdistributed_supervised_learning.so");
    dml_class_registry::get_instance().register_model<logistic_regression>();
    working_dir = turi::get_temp_name();
    fileio::create_directory(working_dir);
  }

  void teardown() {
    fileio::delete_path_recursive(working_dir);
  }

  void test_impl(std::map<std::string, flexible_type> opts, size_t num_workers) {

    setup();

    try {
      // Arrange
      // ----------------------------------------------------------------------
      size_t examples = opts.at("examples");
      size_t features = opts.at("features");

      // Coefficients 
      DenseVector coefs(features+1);
      coefs.randn();
      
      // Feature names
      std::string feature_types;
      for(size_t i=0; i < features; i++){
        feature_types += "n";
      }

      // Generate some data.
      sframe data = make_random_sframe(examples, feature_types, true);
      // Binary target.
      sframe _y = data.select_columns({"target"});
      std::shared_ptr<unity_sframe> _uy(new unity_sframe());
      _uy->construct_from_sframe(_y);
      gl_sframe gl_y(_uy);
      gl_y["target"] = gl_y["target"] > gl_y["target"].mean();

      // Make the data into the right format.
      sframe y = (*gl_y.get_proxy()->get_underlying_sframe());
      sframe X = data;
      X = X.remove_column(X.column_index("target"));

      // Setup the arguments. 
      std::map<std::string, flexible_type> options = { 
        {"convergence_threshold", 1e-2},
        {"step_size", 1.0},
        {"lbfgs_memory_level", 3},
        {"max_iterations", 10},
        {"solver", "newton"},
        {"l1_penalty", 0.0},
        {"l2_penalty", 0.0}
      };
      variant_map_type params;
      std::shared_ptr<unity_sframe> uX(new unity_sframe());
      std::shared_ptr<unity_sframe> uy(new unity_sframe());
      uX->construct_from_sframe(X);
      uy->construct_from_sframe(y);
      params["model_name"] = std::string("classifier_logistic_regression");
      params["features"] = to_variant(uX);
      params["target"] = to_variant(uy);
      for (const auto& kvp: options){
        params[kvp.first] = to_variant(kvp.second);
      }

      // Act
      // ----------------------------------------------------------------------
      // Train the model. 
      variant_type ret = runner.run("distributed_supervised_train", params, working_dir, num_workers);
      std::shared_ptr<logistic_regression> model =
            variant_get_value<std::shared_ptr<logistic_regression>>(ret);

      // Assert
      // ----------------------------------------------------------------------
      // Check options.
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

BOOST_FIXTURE_TEST_SUITE(_distributed_logistic_regression_test, distributed_logistic_regression_test)
BOOST_AUTO_TEST_CASE(test_logistic_regression_basic_2d) {
  distributed_logistic_regression_test::test_logistic_regression_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_logistic_regression_small) {
  distributed_logistic_regression_test::test_logistic_regression_small();
}
BOOST_AUTO_TEST_SUITE_END()
