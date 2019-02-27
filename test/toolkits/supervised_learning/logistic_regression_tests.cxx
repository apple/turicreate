#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <stdlib.h>
#include <vector>
#include <string>
#include <functional>
#include <random>
#include <cfenv>
#include <cmath>

#include <ml_data/ml_data.hpp>
#include <optimization/optimization_interface.hpp>
#include <optimization/utils.hpp>
#include <unity/toolkits/supervised_learning/logistic_regression.hpp>
#include <unity/toolkits/supervised_learning/logistic_regression_opt_interface.hpp>
#include <sframe/testing_utils.hpp>


using namespace turi;
using namespace turi::supervised;

void run_logistic_regression_test(std::map<std::string, flexible_type> opts) {


  size_t examples = opts.at("examples");
  size_t features = opts.at("features");
  std::string target_column_name = "target";

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
    if (i == 0) c = 0; // Make sure category 0 is category 0 (for testing)
    std::vector<flexible_type> y_tmp;
    y_tmp.push_back(c);

    X_data.push_back(x_tmp);
    y_data.push_back(y_tmp);
  }

  // Options
  std::map<std::string, flexible_type> options = {
    {"convergence_threshold", 1e-2},
    {"step_size", 1.0},
    {"lbfgs_memory_level", 3},
    {"max_iterations", 10},
    {"l1_penalty", 0.0},
    {"l2_penalty", 1e-2}
  };

  // Make the data
  sframe X = make_testing_sframe(feature_names, feature_types, X_data);
  sframe y = make_testing_sframe({"target"}, {flex_type_enum::STRING}, y_data);
  std::shared_ptr<logistic_regression> model;
  model.reset(new logistic_regression);
  model->init(X,y);
  model->init_options(options);
  model->train();

  // Construct the ml_data
  ml_data data = model->construct_ml_data_using_current_metadata(X, y);

  // Check coefficients & options
  // ----------------------------------------------------------------------
  DenseVector _coefs(features+1);
  model->get_coefficients(_coefs);
  TS_ASSERT(_coefs.size() == features + 1);

  std::map<std::string, flexible_type> _options;
  _options = model->get_current_options();
  for (auto& kvp: options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }
  TS_ASSERT(model->is_trained() == true);

  // Check predictions
  // ----------------------------------------------------------------------
  std::vector<flexible_type> pred_margin;
  std::shared_ptr<sarray<flexible_type>> _pred_margin
    = model->predict(data, "margin");
  std::vector<flexible_type> pred_class;
  std::shared_ptr<sarray<flexible_type>> _pred_class
    = model->predict(data, "class");
  std::vector<flexible_type> pred_prob;
  std::shared_ptr<sarray<flexible_type>> _pred_prob
    = model->predict(data, "probability");

  // Save predictions made by the model
  auto reader = _pred_margin->get_reader();
  reader->read_rows(0, examples, pred_margin);
  reader = _pred_class->get_reader();
  reader->read_rows(0, examples, pred_class);
  reader = _pred_prob->get_reader();
  reader->read_rows(0, examples, pred_prob);

  // Check that the predictions made by the model are right!
  for(size_t i=0; i < examples; i++){
    DenseVector x(features + 1);
    for(size_t k=0; k < features; k++){
      x(k) = X_data[i][k];
    }
    x(features) = 1;
    double t = x.dot(_coefs);
    double p = pred_margin[i];
    TS_ASSERT(abs(p - t) < 1e-5);
    t = 1.0/(1.0+exp(-1.0*t));
    p = pred_prob[i];
    TS_ASSERT(abs(p - t) < 1e-5);
    int c = t >= 0.5;
    TS_ASSERT_EQUALS(pred_class[i], std::to_string(c));
  }


  // Check save and load
  // ----------------------------------------------------------------------
  dir_archive archive_write;
  archive_write.open_directory_for_write("regr_logistic_regression_tests");
  turi::oarchive oarc(archive_write);
  oarc << *model;
  archive_write.close();

  // Load it
  dir_archive archive_read;
  archive_read.open_directory_for_read("regr_logistic_regression_tests");
  turi::iarchive iarc(archive_read);
  iarc >> *model;


  // Check coefficients after saving and loading.
  // ----------------------------------------------------------------------
  DenseVector _coefs_after_load(features+1);
  model->get_coefficients(_coefs_after_load);
  TS_ASSERT(_coefs_after_load.size() == features + 1);
  TS_ASSERT(_coefs_after_load.isApprox(_coefs, 1e-5));
  _options = model->get_current_options();
  for (auto& kvp: options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }
  TS_ASSERT(model->is_trained() == true);


  // Check coefficients after saving and loading.
  // ----------------------------------------------------------------------
  _pred_margin = model->predict(data, "margin");
  _pred_class = model->predict(data, "class");
  _pred_prob = model->predict(data, "probability");
  reader = _pred_margin->get_reader();
  reader->read_rows(0, examples, pred_margin);
  reader = _pred_class->get_reader();
  reader->read_rows(0, examples, pred_class);
  reader = _pred_prob->get_reader();
  reader->read_rows(0, examples, pred_prob);

  // Check that the predictions made by the model are right!
  for(size_t i=0; i < examples; i++){
    DenseVector x(features + 1);
    for(size_t k=0; k < features; k++){
      x(k) = X_data[i][k];
    }
    x(features) = 1;
    double t = x.dot(_coefs);
    double p = pred_margin[i];
    TS_ASSERT(abs(p - t) < 1e-5);

    t = 1.0/(1.0+exp(-1.0*t));
    p = pred_prob[i];
    TS_ASSERT(abs(p - t) < 1e-5);
    int c = t > 0.5;
    TS_ASSERT_EQUALS(pred_class[i], std::to_string(c));
  }

  model->get_coefficients(_coefs);
  TS_ASSERT(_coefs.size() == features + 1);
  model.reset();


  // Check that we can train a model when providing a validation set
  model.reset(new logistic_regression);
  logprogress_stream << "Training with a validation set" << std::endl;
  model->init(X, y, X, y);
  model->init_options(options);
  model->train();

}

/**
 *  Check logistic regression
*/
struct logistic_regression_test  {
  public:

  void test_logistic_regression_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 100},
      {"features", 1}};
    run_logistic_regression_test(opts);
  }

  void test_logistic_regression_small() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 1000},
      {"features", 10}};
    run_logistic_regression_test(opts);
  }

};



void run_logistic_regression_opt_interface_test(std::map<std::string,
    flexible_type> opts) {


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
    {"convergence_threshold", 1e-2},
    {"step_size", 1.0},
    {"lbfgs_memory_level", 3},
    {"mini_batch_size", 1},
    {"max_iterations", 10},
    {"solver", "auto"},
  };


  // Construct the ml_data
  // Make the data
  sframe X = make_testing_sframe(feature_names, feature_types, X_data);
  sframe y = make_testing_sframe({"target"}, {flex_type_enum::STRING}, y_data);
  std::shared_ptr<logistic_regression> model;
  model.reset(new logistic_regression);
  model->init(X, y);

  // Construct the ml_data
  ml_data data = model->construct_ml_data_using_current_metadata(X, y);
  ml_data valid_data;

  std::shared_ptr<logistic_regression_opt_interface> lr_interface;
  lr_interface.reset(new logistic_regression_opt_interface(data, valid_data, *model));

  // Check examples & variables.
  TS_ASSERT(lr_interface->num_variables() == features + 1);
  TS_ASSERT(lr_interface->num_examples() == examples);

  size_t variables = lr_interface->num_variables();
  for(size_t i=0; i < 10; i++){

    DenseVector point(variables);
    point.setRandom();

    // Check gradients, functions and hessians.
    DenseVector gradient(variables);
    double func_value;
    DenseMatrix hessian(variables, variables);

    func_value = lr_interface->compute_function_value(point);
    lr_interface->compute_gradient(point, gradient);
    lr_interface->compute_hessian(point, hessian);
    TS_ASSERT(check_gradient(*lr_interface, point, gradient));
    if( variables <= 2){
      TS_ASSERT(check_hessian(*lr_interface, point, hessian));
    }


    // Check first order & second order computations
    DenseVector _gradient(variables);
    double _func_value;
    DenseMatrix _hessian(variables, variables);

    lr_interface->compute_first_order_statistics(point, _gradient,
      _func_value);
    TS_ASSERT(abs(func_value - _func_value) < 1e-5);
    TS_ASSERT(gradient.isApprox(_gradient));
    lr_interface->compute_second_order_statistics(point, _hessian, _gradient,
      _func_value);
    TS_ASSERT(abs(func_value - _func_value) < 1e-5);
    TS_ASSERT(gradient.isApprox(_gradient));
    TS_ASSERT(hessian.isApprox(_hessian));

  }

  model.reset();
  lr_interface.reset();
}


/**
 *  Check logistic regression opt interface
*/
struct logistic_regression_opt_interface_test  {
  public:

  void test_logistic_regression_opt_interface_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 100},
      {"features", 1}};
    run_logistic_regression_opt_interface_test(opts);
  }

  void test_logistic_regression_opt_interface_small() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 1000},
      {"features", 10}};
    run_logistic_regression_opt_interface_test(opts);
  }

};

BOOST_FIXTURE_TEST_SUITE(_logistic_regression_test, logistic_regression_test)
BOOST_AUTO_TEST_CASE(test_logistic_regression_basic_2d) {
  logistic_regression_test::test_logistic_regression_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_logistic_regression_small) {
  logistic_regression_test::test_logistic_regression_small();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_logistic_regression_opt_interface_test, logistic_regression_opt_interface_test)
BOOST_AUTO_TEST_CASE(test_logistic_regression_opt_interface_basic_2d) {
  logistic_regression_opt_interface_test::test_logistic_regression_opt_interface_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_logistic_regression_opt_interface_small) {
  logistic_regression_opt_interface_test::test_logistic_regression_opt_interface_small();
}
BOOST_AUTO_TEST_SUITE_END()
