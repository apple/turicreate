/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
///// SDK
//#include <model_server/lib/toolkit_function_macros.hpp>

// Data structures
#include <core/data/flexible_type/flexible_type_base_types.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <model_server/lib/variant.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_iterators.hpp>

// Unity library
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/toolkit_function_specification.hpp>

// Toolkits
#include <toolkits/evaluation/unity_evaluation.hpp>
#include <toolkits/evaluation/metrics.hpp>
#include <toolkits/evaluation/evaluation_constants.hpp>


/// SDK
#include <model_server/lib/toolkit_function_macros.hpp>

#include <core/export.hpp>

namespace turi {
namespace evaluation {


/**
 * Compute the precision/recall at a set of cutoffs for each user.
 */
toolkit_function_response_type precision_recall_by_user(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;
  sframe data = *(std::static_pointer_cast<unity_sframe>(safe_varmap_get<std::shared_ptr<unity_sframe_base>>(invoke.params, "data"))->get_underlying_sframe());
  sframe recommendations = *(std::static_pointer_cast<unity_sframe>(safe_varmap_get<std::shared_ptr<unity_sframe_base>>(invoke.params, "recommendations"))->get_underlying_sframe());
  const flex_vec& cutoffs = safe_varmap_get<flexible_type>(invoke.params, "cutoffs").get<flex_vec>();
  std::vector<size_t> cutoffs_vec;
  cutoffs_vec.reserve(cutoffs.size());
  for (const auto& cutoff : cutoffs) {
    cutoffs_vec.push_back((size_t) cutoff);
  }

  // Create ml_data object
  std::string user_column = recommendations.column_name(USER_COLUMN_INDEX);
  std::string item_column = recommendations.column_name(ITEM_COLUMN_INDEX);
  if (user_column == item_column)
    log_and_throw("User column and item column must be different.");

  sframe pr = precision_recall_by_user(
      data.select_columns({user_column, item_column}),
      recommendations.select_columns({user_column, item_column}),
      cutoffs_vec);

  std::shared_ptr<unity_sframe> pr_sf(new unity_sframe);
  pr_sf->construct_from_sframe(pr);
  ret_status.params["pr"] = to_variant(pr_sf);
  ret_status.success = true;
  return ret_status;
}


/**:
 * Obtains the registration for the  evaluation Toolkit.
 */
EXPORT std::vector<toolkit_function_specification> get_toolkit_function_registration() {

  // Specs
  std::vector<toolkit_function_specification>  specs;

  toolkit_function_specification precision_recall_by_user_spec;
  precision_recall_by_user_spec.name = "evaluation_precision_recall_by_user";
  precision_recall_by_user_spec.toolkit_execute_function =
 (toolkit_function_response_type(*)(toolkit_function_invocation&))precision_recall_by_user;

  specs.push_back(precision_recall_by_user_spec);
  REGISTER_FUNCTION(_supervised_streaming_evaluator, "unity_targets",
                        "unity_predictions", "metric", "kwargs");
  REGISTER_FUNCTION(compute_classifier_metrics, "data", "target", "metric",
                    "prediction", "options");
  REGISTER_FUNCTION(compute_object_detection_metrics, "data",
                    "annotations_column_name", "image_column_name",
                    "prediction", "options");
  return specs;
}


}// evaluation
}// turicreate
