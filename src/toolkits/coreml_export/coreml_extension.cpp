/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_macros.hpp>
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/xgboost.hpp>
#include <toolkits/supervised_learning/linear_svm.hpp>
#include <toolkits/supervised_learning/logistic_regression.hpp>
#include <toolkits/supervised_learning/linear_regression.hpp>
#include <toolkits/coreml_export/linear_models_exporter.hpp>
#include <toolkits/coreml_export/xgboost_exporter.hpp>

using namespace turi;
using namespace turi::supervised;
using namespace turi::supervised::xgboost;

/** Tree models
 *
 */
void _xgboost_export_as_model_asset(
  std::shared_ptr<supervised_learning_model_base> model,
  std::string filename,
  std::map<std::string, flexible_type> context) {

  auto bst_model = std::dynamic_pointer_cast<xgboost_model>(model);

  if (bst_model == nullptr) {
    log_and_throw("Invalid model type. Expect tree models.");
  }

  auto metadata = bst_model->get_ml_metadata();

  flex_list tree_fl = bst_model->get_trees().get<flex_list>();
  std::vector<std::string> trees(tree_fl.begin(), tree_fl.end());

  bool is_classification;

  if(context.at("mode") == "classification") {
    is_classification = true;
  } else if(context.at("mode") == "regression") {
    is_classification = false;
  } else {
    log_and_throw("Internal error: Specified mode must be "
                  "either 'classification' or 'regression'.");
  }

  bool is_random_forest = bst_model->is_random_forest();
  export_xgboost_model(filename, metadata, trees,
      is_classification, is_random_forest, context);
}


/** Linear SVMs
 *
 */
void _linear_svm_export_as_model_asset(
    std::shared_ptr<supervised_learning_model_base> model,
    std::string filename,
    std::map<std::string, flexible_type> context) {

  auto lr_model = std::dynamic_pointer_cast<linear_svm>(model);
  if (lr_model == nullptr) {
    log_and_throw("Invalid model type. Expected linear regression models.");
  }
  auto metadata = lr_model->get_ml_metadata();

  Eigen::Matrix<double, Eigen::Dynamic,1> coefs;

  lr_model->get_coefficients(coefs);
  export_linear_svm_as_model_asset(filename, metadata, coefs, context);
}

void _logistic_classifier_export_as_model_asset(std::shared_ptr<supervised_learning_model_base> model,
                                                std::string filename,
                                                std::map<std::string, flexible_type> context) {

  auto logistic_model = std::dynamic_pointer_cast<logistic_regression>(model);
  if (logistic_model == nullptr) {
    log_and_throw("Invalid model type. Expected logistic classification models.");
  }
  auto metadata = logistic_model->get_ml_metadata();

  Eigen::Matrix<double, Eigen::Dynamic,1> coefs;

  logistic_model->get_coefficients(coefs);
  export_logistic_model_as_model_asset(filename, metadata, coefs, context);
}

void _linear_regression_export_as_model_asset(std::shared_ptr<supervised_learning_model_base> model,
                                              std::string filename,
                                              std::map<std::string, flexible_type> context) {

   auto lr_model = std::dynamic_pointer_cast<linear_regression>(model);
   if (lr_model == nullptr) {
     log_and_throw("Invalid model type. Expected linear regression models.");
   }
  Eigen::Matrix<double, Eigen::Dynamic,1> coefs;
  auto metadata = lr_model->get_ml_metadata();

  lr_model->get_coefficients(coefs);
  export_linear_regression_as_model_asset(filename, metadata, coefs, context);
}

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_xgboost_export_as_model_asset,
    "model", "filename", "context");
REGISTER_FUNCTION(_linear_svm_export_as_model_asset, "model", "filename", "context");
REGISTER_FUNCTION(_logistic_classifier_export_as_model_asset,
    "model", "filename", "context");
REGISTER_FUNCTION(_linear_regression_export_as_model_asset, "model", "filename", "context");
END_FUNCTION_REGISTRATION
