/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/toolkits/supervised_learning/supervised_learning.hpp>
#include <unity/toolkits/supervised_learning/xgboost.hpp>

using namespace turi;
using namespace turi::supervised;
using namespace turi::supervised::xgboost;

//// Supervised learning extensions ////
/**
 * Get feature importance for boosted trees model 
 */
gl_sframe _xgboost_feature_importance(
  std::shared_ptr<supervised_learning_model_base> model) {
  auto bst_model = std::dynamic_pointer_cast<xgboost_model>(model);
  if (bst_model == nullptr) {
    log_and_throw("Invalid model type. Expect tree models.");
  }
  return bst_model->get_feature_importance();
}

/**
 * Get feature importance for boosted trees model 
 */
std::string _xgboost_get_tree(
  std::shared_ptr<supervised_learning_model_base> model, 
  size_t tree_id) {
  auto bst_model = std::dynamic_pointer_cast<xgboost_model>(model);
  if (bst_model == nullptr) {
    log_and_throw("Invalid model type. Expect tree models.");
  }
  return bst_model->get_tree(tree_id);
}

/**
 * Get feature importance for boosted trees model 
 */
std::vector<std::string> _xgboost_dump_model(
  std::shared_ptr<supervised_learning_model_base> model,
  bool with_stats, std::string format) {
  auto bst_model = std::dynamic_pointer_cast<xgboost_model>(model);
  if (bst_model == nullptr) {
    log_and_throw("Invalid model type. Expect tree models.");
  }
  if (format == "text") {
    return bst_model->dump(with_stats);
  } else if (format == "json") {
    return bst_model->dump_json(with_stats);
  } else {
    log_and_throw("Unknown format");
  }
}


BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_xgboost_feature_importance, "model");
REGISTER_FUNCTION(_xgboost_dump_model, "model", "with_stats", "format");
REGISTER_FUNCTION(_xgboost_get_tree, "model", "tree_id");
END_FUNCTION_REGISTRATION
