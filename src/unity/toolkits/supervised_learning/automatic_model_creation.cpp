#include <unity/lib/api/unity_sframe_interface.hpp>
#include <unity/toolkits/supervised_learning/automatic_model_creation.hpp>
#include <unity/toolkits/supervised_learning/boosted_trees.hpp>
#include <unity/toolkits/supervised_learning/decision_tree.hpp>
#include <unity/toolkits/supervised_learning/linear_regression.hpp>
#include <unity/toolkits/supervised_learning/linear_svm.hpp>
#include <unity/toolkits/supervised_learning/logistic_regression.hpp>
#include <unity/toolkits/supervised_learning/random_forest.hpp>
#include <unity/toolkits/supervised_learning/supervised_learning.hpp>


namespace turi {
namespace supervised {

std::shared_ptr<supervised_learning_model_base> create_classifier(const std::string& model_name) {
  std::shared_ptr<supervised_learning_model_base> result;
  if(model_name == "boosted_trees_classifier") {
    result = std::make_shared<xgboost::boosted_trees_classifier>();
  } else if (model_name == "random_forest_classifier") {
    result = std::make_shared<xgboost::random_forest_classifier>();
  } else if (model_name == "decision_tree_classifier") {
    result = std::make_shared<xgboost::decision_tree_classifier>();
  } else if (model_name == "classifier_logistic_regression") {
    result = std::make_shared<logistic_regression>();
  } else if (model_name == "classifier_svm") {
    result = std::make_shared<linear_svm>();
  } else {
    std::ostringstream error_msg;
    error_msg << "Unrecognized Model: " << model_name;
    log_and_throw(error_msg.str().c_str());
  }
  return result;
}


// Return the validation accuracy if it exists, otherwise return training accuracy.
double get_classifier_accuracy(std::shared_ptr<supervised_learning_model_base> model) {
  flexible_type accuracy = FLEX_UNDEFINED;

  // Get the model's progress SFrame.
  variant_type progress = model->get_value_from_state("progress");
  assert(get_variant_which_name(progress.which()) == "SFrame");
  auto progress_sframe = variant_get_ref< std::shared_ptr<turi::unity_sframe_base> >(progress);

  // Get the last validation from the progress SFrame
  std::vector<std::string> column_names = progress_sframe->column_names();
  std::shared_ptr<unity_sarray_base> accuracy_column = NULL;
  if(std::find(column_names.begin(), column_names.end(), "Validation-accuracy") != column_names.end()) {
    accuracy_column = progress_sframe->select_column("Validation-accuracy");
  } else {
    accuracy_column = progress_sframe->select_column("Training-accuracy");
  }
  accuracy = accuracy_column->_tail(1)[0];

  // Sanity check accuracy
  if(accuracy == FLEX_UNDEFINED) {
    log_and_throw("Model does not have metrics that can be used for model selection.");
  }
  double result = accuracy.to<double>();
  assert(result >= 0);
  assert(result <= 1);

  return result;
}


std::shared_ptr<supervised_learning_model_base> create_automatic_classifier_model(
    gl_sframe data, const std::string target, gl_sframe validation_data,
    const std::map<std::string, flexible_type>& options) {

  size_t num_classes = data[target].unique().size();
  std::vector<std::string> possible_models = _classifier_available_models(num_classes, data);

  // If no validation set and enough training data, create a validation set.
  if(validation_data.empty() && data.size() >= 100) {
    std::pair<gl_sframe, gl_sframe>  split = data.random_split(.95);
    data = split.first;
    validation_data = split.second;
  }

  // Train each model. Save the model and its accuracy.
  double accuracies[possible_models.size()];
  std::shared_ptr<supervised_learning_model_base> models[possible_models.size()];
  for(size_t i = 0; i < possible_models.size(); i++) {
    std::shared_ptr<supervised_learning_model_base> cur_model = create_classifier(possible_models[i]);
    cur_model->api_train(data, target, validation_data, options);
 
    accuracies[i] = get_classifier_accuracy(cur_model);
    models[i] = cur_model;
  }

  int best_model = 0;
  for(size_t i = 1; i < possible_models.size(); i++) {
    if(accuracies[i] > accuracies[best_model]) {
      best_model = i;
    }
  }
  
  return models[best_model];
}

std::shared_ptr<supervised_learning_model_base> create_automatic_regression_model(
    gl_sframe data, const std::string target, gl_sframe validation_data,
    const std::map<std::string, flexible_type>& options) {

  std::string model_name = _regression_model_selector(data);

  // If the data is more than 1e5 rows, sample 1e5 rows.
  gl_sframe train_sframe;
  if (data.size() > 1e5) {
    float fraction = 1e5 / data.size();
    train_sframe = data.sample(fraction, 0);
  } else {
    train_sframe = data;
  }

  std::shared_ptr<supervised_learning_model_base> m;
  if(model_name == "boosted_trees_regression") {
    m = std::make_shared<xgboost::boosted_trees_regression>();
  } else {
    DASSERT_TRUE(model_name == "regression_linear_regression");
    m = std::make_shared<linear_regression>();
  }
  //m->init_options(options);
  m->api_train(train_sframe, target, validation_data, options);

  return m; 
}


}
}
