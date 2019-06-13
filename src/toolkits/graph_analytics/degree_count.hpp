/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_specification.hpp>
#ifndef TURI_UNITY_DEGREE_COUNT
#define TURI_UNITY_DEGREE_COUNT

namespace turi {
namespace degree_count {

/**
 * Obtains the registration for the Degree Count Toolkit.
 *
 * <b> Toolkit Name: degree_count </b>
 *
 * Accepted Parameters: None
 *
 * Returned Parameters:
 * \li \b training_time (flexible_type: float). The training time of the algorithm in seconds
 * excluding all other preprocessing stages.
 *
 * \li \b __graph__ (unity_graph). The graph object with the field "in_degree",
 * "out_degree", "total_degree".
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace degree_count
} // namespace turi
#endif
