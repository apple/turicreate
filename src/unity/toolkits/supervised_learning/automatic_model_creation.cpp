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
  variant_type accuracy = NULL;
  std::vector<std::string> model_fields = model->list_fields();
  if(std::find(model_fields.begin(), model_fields.end(), "validation_accuracy") != model_fields.end()) {
    accuracy = model->get_value_from_state("validation_accuracy");
  } else {
    try {
      accuracy = model->get_value_from_state("training_accuracy");
    } catch(...) {
      log_and_throw("Model does not have metrics that can be used for model selection.");
    }
  }

  double result = variant_get_value<flexible_type>(accuracy).to<double>();
  ASSERT_TRUE(result >= 0);
  ASSERT_TRUE(result <= 1);

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
  m->api_train(train_sframe, target, validation_data, options);

  return m; 
}


}
}
