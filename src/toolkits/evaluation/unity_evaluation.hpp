/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_EVALUATION_H
#define TURI_UNITY_EVALUATION_H

#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/unity_base_types.hpp>

namespace turi {
namespace evaluation {




/**
 * Compute precision/recall for each user given a set of recommendations.
 * This is specific to the recommender toolkit.
 * See unity/toolkit/evaluation/metrics.hpp.
 */
toolkit_function_response_type precision_recall_by_user(toolkit_function_invocation& invoke);

std::vector<toolkit_function_specification> get_toolkit_function_registration();

}
}

#endif /* TURI_UNITY_EVALUATION_H */
