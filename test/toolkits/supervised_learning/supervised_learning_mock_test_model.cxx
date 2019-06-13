#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <stdlib.h>
#include <vector>
#include <string>
#include <functional>
#include <random>
#include <cfenv>

#include <ml/ml_data/ml_data.hpp>
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>

using namespace turi;
using namespace turi::supervised;

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;

/**
 * Supervised_learning model example toolkit
 * ---------------------------------------------
 *
 * This example class also serves as test cases for the supervised learning
 * base class. Copy the class verbatim and change things for your
 * supervised learning goodness. Go parametric stats!
 *
 * Methods that must be implemented
 * -----------------------------------------
 *
 * Functions that should always be implemented.
 *
 *  supervised_learning_clone
 *  name
 *  train
 *  predict_single_example
 *  set_default_evaluation_metric
 *  save
 *  load
 *  init_options
 *
 * This class interfaces with the SupervisedLearning class in Python and works
 * end to end once the following set of fuctions are implemented by the user.
 *
 *
 * Example Class
 * -----------------------------------------------------------------------------
 *
 * This class does the wonderously complicated task of predicting constant, no
 * matter what you give it. The train time for this model is super fast
 * but the predictions are only accurate if you are trying to a constant
 * all the time.
 *
 */
class predict_constant : public supervised_learning_model_base {

  double constant = 1;                    /**< Prediction */

  public:

  /**
   * Methods that must be implemented in a new supervised_learning model.
   * -------------------------------------------------------------------------
   */

  BEGIN_CLASS_MEMBER_REGISTRATION("predict_constant");
  IMPORT_BASE_CLASS_REGISTRATION(supervised_learning_model_base);
  END_CLASS_MEMBER_REGISTRATION
  
  /**
   * Train a supervised_learning model.
   */
  void train() override {
    constant = options.value("constant");
  }

  bool is_classifier() const override { return false; }


  /**
   * Set one of the options in the model. Use the option manager to set
   * these options. If the option does not satisfy the conditions that the
   * option manager has imposed on it. Errors will be thrown.
   *
   * \param[in] options Options to set
   */
  void init_options(const std::map<std::string, flexible_type>&_options) override {

    options.create_real_option(
        "constant",
        "Constant that you want us to predict",
        0,
        0,
        1,
        false);

    options.create_categorical_option(
        "solver",
        "Solver used for training",
        "auto",
        {flexible_type("auto")},
        false);

    options.create_integer_option(
        "max_iterations",
        "Maximum number of iterations to perform during training",
        10,
        1,
        1000,
        false);

    // Set options!
    options.set_options(_options);
    add_or_update_state(flexmap_to_varmap(options.current_option_values()));
  }

  flexible_type predict_single_example(
    const DenseVector& x,
    const prediction_type_enum& output_type) override {

    return constant;
  }

  flexible_type predict_single_example(
    const SparseVector& x,
    const prediction_type_enum& output_type) override {

    return constant;
  }


  size_t get_version() const override {
    return 0;
  }

  /**
   * Save the object using Turi's oarc.
   */
  void save_impl(turi::oarchive& oarc) const override {

    variant_deep_save(state, oarc);

    // Everything else
    oarc << ml_mdata
         << metrics
         << constant
         << options;
  }


  /**
   * Load the object using Turi's iarc.
   */
  void load_version(turi::iarchive& iarc, size_t version) override {

    // State
    variant_deep_load(state, iarc);

    // Everything else
    iarc >> ml_mdata
         >> metrics
         >> constant
         >> options;

  }

  /**
  *  Helper functions for this class
  * -------------------------------------------------------------------------
  */

  double get_constant(){
    return constant;
  }

  std::shared_ptr<coreml::MLModelWrapper> export_to_coreml() override {
    return std::shared_ptr<coreml::MLModelWrapper>();
  }
};



void run_predict_constant_test(std::map<std::string, flexible_type> opts) {


  size_t examples = opts.at("examples");
  size_t features = opts.at("features");
  std::string target_column_name = "target";
  std::vector<std::string> column_names = {"user", "item"};

  // Answers
  // -----------------------------------------------------------------------
  DenseVector coefs(features);
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
    DenseVector x((size_t) features);
    x.setRandom();
    std::vector<flexible_type> x_tmp;
    for(size_t k=0; k < features; k++){
      x_tmp.push_back(x(k));
    }

    // Compute the prediction for this
    double t = x.dot(coefs);
    std::vector<flexible_type> y_tmp;
    y_tmp.push_back(t);
    X_data.push_back(x_tmp);
    y_data.push_back(y_tmp);
  }

  // Options
  std::map<std::string, flexible_type> options = {
    {"max_iterations", 1},
    {"solver", "auto"},
    {"constant", 0}
  };
  // Options
  std::map<std::string, flexible_type> default_options = {
    {"max_iterations", 10},
    {"solver", "auto"},
    {"constant", 0}
  };

  // Make the data
  sframe X = make_testing_sframe(feature_names, feature_types, X_data);
  sframe y = make_testing_sframe({"target"}, {flex_type_enum::INTEGER}, y_data);
  std::shared_ptr<predict_constant> model;
  model.reset(new predict_constant);

  // Init and train
  model->init(X,y);
  model->init_options(options);
  model->train();

  // Variables for testing
  // --------------------------------------------------------------------------
  std::map<std::string, flexible_type> _options;
  flexible_type _get;
  sframe test_data;
  std::vector<std::string> _list_fields;
  std::vector<std::string> _list_fields_ans = {
        "constant",
        "features",
        "num_examples",
        "num_features",
        "target",
        "constant",
        "features",
        "num_examples",
        "num_features",
        "target"
  };

  flexible_type _features;
  flexible_type _examples;
  flexible_type _target;
  ml_data data;
  std::vector<flexible_type> _pred;
  std::shared_ptr<sarray<flexible_type>> pred;


  // Check the model
  // ----------------------------------------------------------------------
  // Constant
  TS_ASSERT(model->get_constant() == options["constant"]);
  _features = boost::get<flexible_type>(model->get_value_from_state("num_features"));
  _examples= boost::get<flexible_type>(model->get_value_from_state("num_examples"));
  _target = boost::get<flexible_type>(model->get_value_from_state("target"));

  TS_ASSERT(_features == features);
  TS_ASSERT(_examples == examples);


  //// Check options
  _options = model->get_current_options();
  for (auto& kvp: options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }

  //// Default options
  _options = model->get_default_options();
  for (auto& kvp: default_options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }

  // Check set options
  model->set_options({{"constant", 1}});
  _get = boost::get<flexible_type>(model->get_value_from_state("constant"));
  TS_ASSERT(_get == 1);
  _get = model->get_option_value("constant");
  TS_ASSERT(_get == 1);
  model->set_options({{"constant", 0}});
  _get = boost::get<flexible_type>(model->get_value_from_state("constant"));
  TS_ASSERT(_get == 0);
  _get = model->get_option_value("constant");
  TS_ASSERT(_get == 0);

  // Check list_fields
  _list_fields = model->list_fields();
  for(const auto& f: _list_fields_ans){
    TS_ASSERT(std::find(_list_fields.begin(), _list_fields.end(), f)
                                                    != _list_fields.end());
  }

  // Check trained
  TS_ASSERT(model->is_trained() == true);

  // Check predictions
  // ----------------------------------------------------------------------
  data = model->construct_ml_data_using_current_metadata(X);
  pred = model->predict(data);
  auto reader = pred->get_reader();
  reader->read_rows(0, examples, _pred);
  for(size_t i=0; i < examples; i++){
    TS_ASSERT(_pred[i] - options["constant"] < 1e-5);
  }

  // Model 2: Loaded model
  // ------------------------------------------------------------------------
  std::shared_ptr<predict_constant> loaded_model;
  loaded_model.reset(new predict_constant);
  dir_archive archive_write;
  archive_write.open_directory_for_write("predict_constant_tests");
  turi::oarchive oarc(archive_write);
  oarc << *model;
  archive_write.close();
  dir_archive archive_read;
  archive_read.open_directory_for_read("predict_constant_tests");
  turi::iarchive iarc(archive_read);
  iarc >> *loaded_model;

  //// Check the loaded and saved model
  //// ----------------------------------------------------------------------
  // Constant
  TS_ASSERT(loaded_model->get_constant() == options["constant"]);
  _features = boost::get<flexible_type>(
                loaded_model->get_value_from_state("num_features"));
  _examples= boost::get<flexible_type>(
                loaded_model->get_value_from_state("num_examples"));
  _target = boost::get<flexible_type>(
                loaded_model->get_value_from_state("target"));

  TS_ASSERT(_features == features);
  TS_ASSERT(_examples == examples);
  TS_ASSERT(_target == loaded_model->get_target_name());


  //// Check options
  _options = loaded_model->get_current_options();
  for (auto& kvp: options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }

  //// Default options
  _options = loaded_model->get_default_options();
  for (auto& kvp: default_options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }

  // Check set options
  loaded_model->set_options({{"constant", 1}});
  _get = boost::get<flexible_type>(
                      loaded_model->get_value_from_state("constant"));
  TS_ASSERT(_get == 1);
  _get = loaded_model->get_option_value("constant");
  TS_ASSERT(_get == 1);
  loaded_model->set_options({{"constant", 0}});
  _get = boost::get<flexible_type>(
                      loaded_model->get_value_from_state("constant"));
  TS_ASSERT(_get == 0);
  _get = loaded_model->get_option_value("constant");
  TS_ASSERT(_get == 0);

  // Check list_fields
  _list_fields = loaded_model->list_fields();
  for(const auto& f: _list_fields_ans){
    TS_ASSERT(std::find(_list_fields.begin(), _list_fields.end(), f)
                            != _list_fields.end());
  }


  // Check trained
  TS_ASSERT(loaded_model->is_trained() == true);

  // Check predictions
  // ----------------------------------------------------------------------
  data = loaded_model->construct_ml_data_using_current_metadata(X);
  pred = loaded_model->predict(data);
  auto reader2 = pred->get_reader();
  reader2->read_rows(0, examples, _pred);
  for(size_t i=0; i < examples; i++){
    TS_ASSERT(_pred[i] - options["constant"] < 1e-5);
  }


}

/**
 *  Check linear supervised
*/
struct predict_constant_test  {

  public:

  void test_predict_constant_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 100},
      {"features", 1}};
    run_predict_constant_test(opts);
  }

  void test_predict_constant_small() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 1000},
      {"features", 10}};
    run_predict_constant_test(opts);
  }

};

BOOST_FIXTURE_TEST_SUITE(_predict_constant_test, predict_constant_test)
BOOST_AUTO_TEST_CASE(test_predict_constant_basic_2d) {
  predict_constant_test::test_predict_constant_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_predict_constant_small) {
  predict_constant_test::test_predict_constant_small();
}
BOOST_AUTO_TEST_SUITE_END()
