#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <stdlib.h>
#include <vector>
#include <string>
#include <functional>

#include <random>

// ML-Data Utils
#include <ml_data/ml_data.hpp>

// Models
#include <unity/toolkits/supervised_learning/random_forest.hpp>

#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::supervised;
using namespace turi::supervised::xgboost;

void run_random_forest_classifier_test(std::map<std::string, flexible_type> opts) {


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
  std::vector<std::vector<flexible_type>> y_data;
  std::vector<std::vector<flexible_type>> X_data;
  for(size_t i=0; i < examples; i++){
    DenseVector x(features);
    x.setRandom();
    std::vector<flexible_type> x_tmp;
    for(size_t k=0; k < features; k++){
      x_tmp.push_back(x(k));
    }

    // Compute the prediction for this
    double t = x.dot(coefs.segment(0, features)) + coefs(features);
    t = 1.0/(1.0+exp(-1.0*t));
    int c = turi::random::bernoulli(t);
    std::vector<flexible_type> y_tmp;
    y_tmp.push_back(c);

    X_data.push_back(x_tmp);
    y_data.push_back(y_tmp);
  }

  // Options
  std::map<std::string, flexible_type> options = {
    {"max_iterations", 10},
    {"column_subsample", 1.0}
  };

  // Make the data
  sframe X = make_testing_sframe(feature_names, feature_types, X_data);
  sframe y = make_testing_sframe({"target"}, {flex_type_enum::INTEGER}, y_data);
  std::shared_ptr<random_forest_classifier> model;
  model.reset(new random_forest_classifier);
  model->init(X,y);
  model->init_options(options);
  model->train();

  // Check options
  // ----------------------------------------------------------------------
  std::map<std::string, flexible_type> _options;
  _options = model->get_current_options();
  for (auto& kvp: options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
    if (kvp.first == "max_iterations") {
      logprogress_stream << "Max iterations should be 10: "
                         << kvp.second
                         << std::endl;
    }
  }
  TS_ASSERT(model->is_trained() == true);

  // Construct the ml_data
  ml_data data = model->construct_ml_data_using_current_metadata(X, y);
  ml_data valid_data;

  // Check predictions
  // ----------------------------------------------------------------------
  std::vector<flexible_type> pred_class;
  std::shared_ptr<sarray<flexible_type>> _pred_class
    = model->predict(data, "class");


  // Check that we can train a model when providing a validation set
  std::vector<std::vector<flexible_type>> y_v;
  std::vector<std::vector<flexible_type>> X_v;
  for(size_t i=0; i < 5; i++){
    DenseVector x(features);
    x.setRandom();
    std::vector<flexible_type> x_tmp;
    for(size_t k=0; k < features; k++){
      x_tmp.push_back(x(k));
    }

    // Compute the prediction for this
    double t = x.dot(coefs.segment(0, features)) + coefs(features);
    t = 1.0/(1.0+exp(-1.0*t));
    int c = turi::random::bernoulli(t);
    std::vector<flexible_type> y_tmp;
    y_tmp.push_back(1 + c);

    X_v.push_back(x_tmp);
    y_v.push_back(y_tmp);
  }

  sframe Xv = make_testing_sframe(feature_names, feature_types, X_v);
  sframe yv = make_testing_sframe({"target"}, {flex_type_enum::INTEGER}, y_v);

  model.reset(new random_forest_classifier);
  logprogress_stream << "Training with a validation set" << std::endl;
  model->init(X, y, Xv, yv);
  model->init_options(options);
  model->train();

}

struct random_forest_classifier_test  {

  public:

  void test_random_forest_classifier_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 100},
      {"features", 1}};
    run_random_forest_classifier_test(opts);
  }

  void test_random_forest_classifier_small() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 1000},
      {"features", 10}};
    run_random_forest_classifier_test(opts);
  }

};

BOOST_FIXTURE_TEST_SUITE(_random_forest_classifier_test, random_forest_classifier_test)
BOOST_AUTO_TEST_CASE(test_random_forest_classifier_basic_2d) {
  random_forest_classifier_test::test_random_forest_classifier_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_random_forest_classifier_small) {
  random_forest_classifier_test::test_random_forest_classifier_small();
}
BOOST_AUTO_TEST_SUITE_END()
