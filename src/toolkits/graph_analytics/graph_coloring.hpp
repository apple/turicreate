/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_specification.hpp>
#ifndef TURI_UNITY_GRAPH_COLORING
#define TURI_UNITY_GRAPH_COLORING

namespace turi {
namespace graph_coloring{
/**
 * Obtains the registration for the Graph Coloring Toolkit
 *
 * Colors the graph such that adjacent vertices do not have the same color.
 * This implements a heuristic coloring and there are no guarantees that it will
 * find the smallest coloring; just a minimal coloring (i.e. there are no
 * local single vertex color changes that will improve the coloring).
 * Consecutive executions may return different colorings.
 *
 * <b> Toolkit Name: graph_coloring </b>
 *
 * Accepted Parameters: None
 *
 * Returned Parameters:
 * \li \b training_time (flexible_type: float). The training time of the algorithm in seconds
 * excluding all other preprocessing stages.
 *
 * \li \b num_colors (flexible_type: int) The total number of colors found
 *
 * \li \b __graph__ (unity_graph). The graph object with the field "component_id",
 * The component_id field (integer) on each vertex contains the component ID
 * of the vertex. All vertices with the same component ID are connected.
 * component IDs are not sequential and can be arbitrary integers.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace graph_coloring
} // namespace turi
#endif
