/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/supervised_learning/decision_tree.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>
#include <xgboost/src/learner/learner-inl.hpp>

namespace turi {
namespace supervised {
namespace xgboost {


/**
 * Init the GLC options manager.
 *
 * \param[in,out] options  GLC options manager
 */
void init_decision_tree_common_options(option_manager &options){
  options.create_integer_option(
      "max_depth",
      "The maximum depth of individual trees",
      6,
      1,
      std::numeric_limits<int>::max(),
      false);

  options.create_real_option(
      "min_child_weight",
      "Minimum weight required on the leaf nodes",
      0.1,
      0.0,
      std::numeric_limits<float>::max(),
      false);

  options.create_real_option(
      "min_loss_reduction",
      "Minimum loss reduction required for splitting a node",
      0.0,
      0.0,
      std::numeric_limits<float>::max(),
      false);

  options.create_integer_option(
      "random_seed",
      "Seed for row and column subselection",
      flex_undefined(),
      std::numeric_limits<int>::min() + 1, // yay! Windows!
      std::numeric_limits<int>::max(),
      false);

  options.create_flexible_type_option(
      "metric",
      "Performance metric(s) to track during training iterations",
      "auto",
      false);
}

/**
 * Init the XGboost options manager.
 *
 * \param[in] options  GLC options manager
 * \param[in,out] booster  XGBoost booster type
 */
void set_xgboost_decision_tree_common_options(
  option_manager options,
  ::xgboost::learner::BoostLearner &booster_){

  booster_.SetParam("max_iterations", "1");
  booster_.SetParam("eta", "1.0");
  for (auto p : options.current_option_values()) {
    const auto &name = p.first;
    std::string value(p.second);
    if (name == "min_loss_reduction") {
      booster_.SetParam("gamma", value.c_str());
    } else if (name == "random_seed"){
      if (p.second.get_type() != flex_type_enum::UNDEFINED){
        booster_.SetParam("seed", value.c_str());
      }
    } else {
      booster_.SetParam(name.c_str(), value.c_str());
    }
  }
}


/**
 * Regression
 * -------------------------------------------------------------------------
 */


void decision_tree_regression::configure(void) {

  booster_->SetParam("silent", "1");

  booster_->SetParam("objective", "reg:linear");
  set_xgboost_decision_tree_common_options(options, *booster_);

  // Display the config script
  display_regression_training_summary("Decision tree regression");
}

/**
 * Set one of the options in the algorithm.
 *
 * This values is checked	against the requirements given by the option
 * instance. Options that are not present use default options.
 *
 * \param[in] opts Options to set
 */
void decision_tree_regression::init_options(
                          const std::map<std::string,flexible_type>& _opts) {
  // base class
  xgboost_model::init_options(_opts);

  init_decision_tree_common_options(options);
  options.set_options(_opts);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

std::shared_ptr<coreml::MLModelWrapper> decision_tree_regression::export_to_coreml() {

  std::map<std::string, flexible_type> context = {
    {"model_type", "decision_tree"},
    {"version", get_version()},
    {"class", name()},
    {"short_description", "Decision Tree Regression model."}};

  return this->_export_xgboost_model(false, true, context);
}

/**
 * classifier
 * -------------------------------------------------------------------------
 */

/**
 * Init function common to all regression inits.
 */
void decision_tree_classifier::model_specific_init(const ml_data& data,
                                                   const ml_data& valid_data){
  xgboost_model::model_specific_init(data, valid_data);
  // Update the model
  state["num_classes"] = this->ml_mdata->target_index_size();
  state["num_examples_per_class"] =
             to_variant(supervised::get_num_examples_per_class(this->ml_mdata));

}



/**
 * Set XGBoost options
 */
void decision_tree_classifier::configure(void) {

  std::stringstream ss;
  size_t num_classes = variant_get_value<size_t>(state.at("num_classes"));

  booster_->SetParam("silent", "1");
  if (num_classes > 2){
    booster_->SetParam("num_class", std::to_string(num_classes).c_str());
    booster_->SetParam("objective", "multi:softprob");
  } else {
    booster_->SetParam("objective", "binary:logistic");
  }
  set_xgboost_decision_tree_common_options(options, *booster_);

  // Display before training
  display_classifier_training_summary("Decision tree classifier");
}

/**
 * Set one of the options in the algorithm.
 *
 * This values is checked	against the requirements given by the option
 * instance. Options that are not present use default options.
 *
 * \param[in] opts Options to set
 */
void decision_tree_classifier::init_options(
                          const std::map<std::string,flexible_type>& _opts) {
  // base class
  xgboost_model::init_options(_opts);

  // Init classifier specific options
  options.create_flexible_type_option(
    "class_weights",
    "Weights (during training) assigned to each class.",
    flex_undefined(),
    true);

  init_decision_tree_common_options(options);
  options.set_options(_opts);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));

}

std::shared_ptr<coreml::MLModelWrapper> decision_tree_classifier::export_to_coreml() {

  std::map<std::string, flexible_type> context = {
    {"model_type", "decision_tree"},
    {"version", std::to_string(get_version())},
    {"class", name()},
    {"short_description", "Decision Tree classification model."}};

  return this->_export_xgboost_model(true, true, context);
}



}  // namespace xgboost
}  // namespace supervised
}  // namespace turi
