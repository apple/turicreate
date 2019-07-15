#include <toolkits/supervised_learning/automatic_model_creation.hpp>

#include <algorithm>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

#include <model_server/lib/api/unity_sframe_interface.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <toolkits/supervised_learning/boosted_trees.hpp>
#include <toolkits/supervised_learning/decision_tree.hpp>
#include <toolkits/supervised_learning/linear_regression.hpp>
#include <toolkits/supervised_learning/linear_svm.hpp>
#include <toolkits/supervised_learning/logistic_regression.hpp>
#include <toolkits/supervised_learning/random_forest.hpp>
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>


namespace turi {
namespace supervised {

namespace {

// Threshold on the number of feature columns, used to determine the
// applicability of certain models.
constexpr size_t WIDE_DATA = 200;

/**
 * Compute the width of the data.
 *
 * \param[in] X  Input SFrame
 * \returns width
 *
 * The width is the same as the num_coefficients.
 *
 */
size_t compute_data_width(sframe X){
  ml_data data;
  data.fill(X);
  return get_number_of_coefficients(data.metadata());
}


// If `field_name` is present in `model_fields`, populates `*output` with the
// value of `model->get_value_from_state(field_name)` and returns
// true. Otherwise, returns false.
bool read_model_field(
    const std::shared_ptr<supervised_learning_model_base>& model,
    const std::vector<std::string>& model_fields,
    const char* field_name, variant_type* output) {
  auto iter = std::find(model_fields.begin(), model_fields.end(), field_name);
  bool success = iter != model_fields.end();
  if (success) {
    *output = model->get_value_from_state(field_name);
  }
  return success;
}

// Returns the regressors worth trying for the given data.
std::vector<std::shared_ptr<supervised_learning_model_base>>
get_regression_models(std::shared_ptr<unity_sframe> X) {

  std::vector<std::shared_ptr<supervised_learning_model_base>> models;

  const size_t data_width = compute_data_width(*X->get_underlying_sframe());

  // Always try linear regression.
  models.push_back(std::make_shared<linear_regression>());

  if (data_width < WIDE_DATA) {
    models.push_back(std::make_shared<xgboost::boosted_trees_regression>());
  }

  return models;
}

// Returns the validation Root Mean Squared Error if it exists, otherwise
// returns training RMSE. Throws if neither field exists.
double get_regression_rmse(
    const std::shared_ptr<supervised_learning_model_base>& model) {

  variant_type rmse;
  std::vector<std::string> model_fields = model->list_fields();
  bool success = false;
  if (!success) {
    success = read_model_field(model, model_fields, "validation_rmse", &rmse);
  }
  if (!success) {
    success = read_model_field(model, model_fields, "training_rmse", &rmse);
  }
  if (!success) {
    log_and_throw("Model does not have metrics that can be used for model "
                  "selection.");
  }

  double result = variant_get_value<flexible_type>(rmse).to<double>();
  ASSERT_TRUE(result >= 0);

  return result;
}

}  // anonymous namespace

/**
 * Rule based better than stupid model selector.
 */
std::string _classifier_model_selector(std::shared_ptr<unity_sframe> _X){

  sframe X = *(_X->get_underlying_sframe());

  size_t data_width = compute_data_width(X);
  if (data_width < WIDE_DATA){
    return "boosted_trees_classifier";
  } else {
    return "classifier_logistic_regression";
  }
}

/**
 * Rule based better than stupid model selector.
 */
std::vector<std::string> _classifier_available_models(size_t num_classes,
                                         std::shared_ptr<unity_sframe> _X){
  sframe X = *(_X->get_underlying_sframe());

  // Throw error if only one class.
  // If number of classes more than 2, use boosted trees
  if (num_classes == 1) {
    log_and_throw("One-class classification is not currently supported. Please check your target column.");
  } else if (num_classes > 2) {
    return {"boosted_trees_classifier",
            "random_forest_classifier",
            "decision_tree_classifier",
            "classifier_logistic_regression"};
  } else {
    size_t data_width = compute_data_width(X);
    if (data_width < WIDE_DATA){
      return {"boosted_trees_classifier",
              "random_forest_classifier",
              "decision_tree_classifier",
              "classifier_svm",
              "classifier_logistic_regression"};
    } else {
      return {"classifier_logistic_regression",
              "classifier_svm"};
    }
  }

  return std::vector<std::string>();
}

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
    gl_sframe data, const std::string target, const variant_type& _validation_data,
    const std::map<std::string, flexible_type>& options) {

  gl_sframe validation_data;
  std::tie(data, validation_data) = create_validation_data(data, _validation_data);

  // TODO: The original Python code path only runs the model selector on at most
  // 100000 rows of data. Should we do the same here?
  size_t num_classes = data[target].unique().size();
  std::vector<std::string> possible_models = _classifier_available_models(num_classes, data);

  // If no validation set and enough training data, create a validation set.
  // TODO: The original Python code path allowed users to specify no validation
  // set (use all data for training) by providing an empty SFrame or None.
  // Should these behaviors be unified?
  if(validation_data.empty() && data.size() >= 100) {
    std::pair<gl_sframe, gl_sframe>  split = data.random_split(.95);
    data = split.first;
    validation_data = split.second;
  }

  // Train each model. Save the model and its accuracy.
  std::vector<double> accuracies(possible_models.size());
  std::vector<std::shared_ptr<supervised_learning_model_base> > models(possible_models.size());
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
    gl_sframe data, const std::string target, const variant_type& _validation_data,
    const std::map<std::string, flexible_type>& options) {

  // Perform training/validation split if necessary.
  gl_sframe validation_data;
  std::tie(data, validation_data) = create_validation_data(data, _validation_data);

  // If the data is more than 1e5 rows, sample 1e5 rows.
  // TODO: The original Python implementation only sampled the training data for
  // the purposes of determining what models to attempt. It used all the
  // provided data for training. Probably this code path should do the same,
  // although the running time of this function could increase dramatically for
  // very large data sets.
  gl_sframe train_sframe;
  if (data.size() > 1e5) {
    float fraction = 1e5 / data.size();
    train_sframe = data.sample(fraction, 0);
  } else {
    train_sframe = data;
  }

  // Determine what regression models to try.
  using shared_model_ptr = std::shared_ptr<supervised_learning_model_base>;
  std::vector<shared_model_ptr> models = get_regression_models(train_sframe);
  ASSERT_GT(models.size(), 0);

  // Perform training on each model.
  for (const shared_model_ptr& model : models) {
    model->api_train(train_sframe, target, validation_data, options);
  }

  // Return the model with the lowest validation (or training) RMSE.
  auto compare_rmse = [](const shared_model_ptr& a, const shared_model_ptr& b) {
    return get_regression_rmse(a) < get_regression_rmse(b);
  };
  return *std::min_element(models.begin(), models.end(), compare_rmse);
}

std::pair<gl_sframe, gl_sframe> create_validation_data(
    gl_sframe data, const variant_type& _validation_data, int random_seed)
{
  if(variant_is<flex_string>(_validation_data)
      && variant_get_value<flex_string>(_validation_data) == "auto") {

    if(data.size() >= 200000) {
      // Aim for 10000 points
      logprogress_stream << "Automatically generating validation set by "
                            "sampling about 10000 out of "
                         << data.size() << " datapoints." << std::endl;

      double p = 10000.0 / data.size();
      return data.random_split(1.0 - p, random_seed);
    } else if(data.size() >= 200) {
      logprogress_stream << "Automatically generating validation set from 5% of the data." << std::endl;
      return data.random_split(0.95, random_seed);
    } else if(data.size() >= 50) {
      logprogress_stream << "Automatically generating validation set from 10% of the data." << std::endl;
      return data.random_split(0.9, random_seed);
    } else {
      logprogress_stream << "Skipping automatic creation of validation set; training set has fewer than 50 points." << std::endl;
      return {data, gl_sframe()};
    }

  } else if (variant_is<gl_sframe>(_validation_data)) {
    return {data, variant_get_value<gl_sframe>(_validation_data)};
  } else {
    log_and_throw("Validation data parameter must be either \"auto\", an empty SFrame "
        "(no validation info is computed), or an SFrame with the same schema as the training data.");
  }
}

std::pair<gl_sframe, gl_sframe> create_validation_data(
    gl_sframe data, const variant_type& _validation_data)
{
  // For now, mimic the behavior of gl_sframe::random_split when no seed is
  // provided.
  // TODO: Should this ultimately use std::random_device instead?
  int random_seed = static_cast<int>(time(nullptr));
  return create_validation_data(std::move(data), _validation_data, random_seed);
}

}  // namespace supervised
}  // namespace turi
