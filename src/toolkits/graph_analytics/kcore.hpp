/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_specification.hpp>
#ifndef TURI_UNITY_KCORE
#define TURI_UNITY_KCORE

namespace turi {
namespace kcore {


/**
 * Obtains the registration for the KCore Toolkit.
 *
 * Performs kcore decomposition on the graph.
 *
 * <b> Toolkit Name: kcore </b>
 *
 * Accepted Parameters:
 * \li \b kmin (flexible_type: integer). The lowest KCore value to compute
 * (inclusive).  Defaults to 0. All vertices with a core value out of this
 * range will have a core ID of -1.
 *
 * \li \b kmax (flexible_type: integer). The highest KCore value to compute
 * (inclusive). Defaults to 100. All vertices with a core value out of this
 * range will have a core ID of -1.
 *
 * Returned Parameters:
 * \li \b training_time (flexible_type: float). The training time of the algorithm in seconds
 * excluding all other preprocessing stages.
 *
 * \li \b max_core The largest core value encountered.
 *
 * \li \b __graph__ (unity_graph). The graph object with the field "core_id",
 * The core_id field (integer) contains the core number of the vertex.
 * This number will be in between kmin amd kmax (inclusive). All vertices with
 * core values outside of this range will have core_id of -1.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace kcore
} // namespace turi
#endif
