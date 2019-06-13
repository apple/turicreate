/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_specification.hpp>
#ifndef TURI_UNITY_TRIANGLE_COUNTING
#define TURI_UNITY_TRIANGLE_COUNTING

namespace turi {
namespace triangle_counting {
/**
 * Obtains the registration for the Triangle Counting Toolkit
 *
 * Counts the number of undirected triangles in the graph.
 * <b> Toolkit Name: triangle_counting </b>
 *
 * Accepted Parameters: None
 *
 * Returned Parameters:
 * \li \b training_time (flexible_type: float). The training time of the algorithm in seconds
 * excluding all other preprocessing stages.
 *
 * \li \b num_triangles (flexible_type: int) The total number of triangles found
 *
 * \li \b __graph__ (unity_graph). The graph object with the field "triangle_count",
 * The triangle_count field (integer) on each vertex contains the number of
 * triangles each vertex is involved in.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace triangle_counting
} // namespace turi
#endif
