/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/ml_data_2/ml_data_missing_values.hpp>

namespace turi { namespace v2 { namespace ml_data_internal {

missing_value_action get_missing_value_action(
    const std::map<std::string, flexible_type>& options,
    bool training_mode) {

  std::string missing_value_action_str
      = options.at(
          training_mode
          ? "missing_value_action_on_train"
          : "missing_value_action_on_predict");

  missing_value_action none_action;

  if(missing_value_action_str == "error") {
    none_action = missing_value_action::ERROR;
  } else if (missing_value_action_str == "impute") {
    none_action = missing_value_action::IMPUTE;
  } else {
    ASSERT_MSG(false, "Missing value action must be either 'error' or 'impute'.");
  }

  // Can't impute with changing means
  ASSERT_MSG(!((none_action == missing_value_action::IMPUTE) &&
               (training_mode == true)),
             "missing_value_action 'impute' and training mode are not compatible.");

  return none_action;
}

}}}
