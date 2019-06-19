#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <stdlib.h>
#include <vector>
#include <string>
#include <functional>

#include <random>

// ML-Data Utils
#include <toolkits/ml_data_1/ml_data.hpp>
#include <toolkits/ml_data_1/metadata.hpp>
#include <toolkits/ml_data_1/sframe_index_mapping.hpp>

// Optimization Interface
#include <ml/optimization/optimization_interface.hpp>
#include <ml/optimization/utils.hpp>

// Models
#include <toolkits/supervised_learning/linear_regression.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::supervised;

void run_linear_regression_test(std::map<std::string, flexible_type> opts) {


  size_t examples = opts.at("examples");
  size_t features = opts.at("features");
  std::string target_column_name = "target";
  std::vector<std::string> column_names = {"user", "item"};

  // Answers
  // -----------------------------------------------------------------------
  DenseVector coefs(features+1);
  coefs.setRandom();

  // Feature names
  std::vector<std::string> feature_names;
  std::vector<flex_type_enum> feature_types;
  for(size_t i=0; i < features; i++){
    feature_names.push_back(std::to_string(i));
    feature_types.push_back(flex_type_enum::FLOAT);
  }

  // Data
  std::vector<std::vector<flexible_type>> X_data;
  std::vector<std::vector<flexible_type>> y_data;
  X_data.resize(examples);
  y_data.resize(examples);
  for(size_t i=0; i < examples; i++){
    X_data[i].resize(features);
    for(size_t k=0; k < features; k++){
      X_data[i][k] = turi::random::normal();
    }
    y_data[i] = flexible_type(turi::random::bernoulli(0.5));
  }

  // Options
  std::map<std::string, flexible_type> options = {
    {"convergence_threshold", 1e-2},
    {"solver", "auto"},
    {"max_iterations", 10},
  };

  // Make the data
  sframe X = make_testing_sframe(feature_names, feature_types, X_data);
  sframe y = make_testing_sframe({"target"}, {flex_type_enum::INTEGER}, y_data);

  std::shared_ptr<linear_regression> model;
  model.reset(new linear_regression);
  model->init(X,y);
  model->init_options(options);
  model->train();
  model.reset();

}

/**
 *  Check linear regression
*/
struct linear_regression_test  {
  public:

  void test_linear_regression_tiny() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 100},
      {"features", 10}};
    run_linear_regression_test(opts);
  }

  void test_linear_regression_small() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 1000000},
      {"features", 10}};
    run_linear_regression_test(opts);
  }

  //void test_linear_regression_medium() {
  //  std::map<std::string, flexible_type> opts = {
  //    {"examples", 1000000},
  //    {"features", 100}};
  //  run_linear_regression_test(opts);
  //}

  //void test_linear_regression_large() {
  //  std::map<std::string, flexible_type> opts = {
  //    {"examples", 10000000},
  //    {"features", 100}};
  //  run_linear_regression_test(opts);
  //}
};

BOOST_FIXTURE_TEST_SUITE(_linear_regression_test, linear_regression_test)
BOOST_AUTO_TEST_CASE(test_linear_regression_tiny) {
  linear_regression_test::test_linear_regression_tiny();
}
BOOST_AUTO_TEST_CASE(test_linear_regression_small) {
  linear_regression_test::test_linear_regression_small();
}
BOOST_AUTO_TEST_SUITE_END()
