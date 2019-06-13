/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_CONNECTED_COMPONENT
#define TURI_UNITY_CONNECTED_COMPONENT
#include <model_server/lib/toolkit_function_specification.hpp>

namespace turi {
namespace connected_component {
/**
 * Obtains the registration for the Connected Component Toolkit.
 *
 * Computes Weakly connected components on the graph
 *
 * <b> Toolkit Name: connected_component </b>
 *
 * Accepted Parameters: None
 *
 * Returned Parameters:
 * \li \b training_time (flexible_type: float). The training time of the algorithm in seconds
 * excluding all other preprocessing stages.
 *
 * \li \b num_of_components (flexible_type: int) The number of components of the graph.
 *
 * \li \b __graph__ (unity_graph). The graph object with the field "component_id",
 * The component_id field (integer) on each vertex contains the component ID
 * of the vertex. All vertices with the same component ID are connected.
 * component IDs are not sequential and can be arbitrary integers.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration() ;

} // namespace connected_component
} // namespace turi
#endif
