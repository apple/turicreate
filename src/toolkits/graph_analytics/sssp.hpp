/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_specification.hpp>
#ifndef TURI_UNITY_SSSP
#define TURI_UNITY_SSSP

namespace turi {
  namespace sssp {
  /**
   * Obtains the registration for the SSSP Toolkit.
   *
   * <b> Toolkit Name: sssp </b>
   *
   * Accepted Parameters:
   * \li \b source_vid (flexible_type). The source vertex to compute SSSP from.
   *
   * \li \b max_dist (flexible_type: float). The largest distance to expand to.
   * (Default 1E30)
   *
   * \li \b edge_attr(flexible_type: string). The attribute to use for
   * edge weights. If empty string, uniform weights are used (every edge has a
   * weight of 1). Otherwise, edge_weight must refer to an edge field with a
   * integer or float value. If any edge does not contain the field, it
   * is assumed to have an an infinite weight.  (Default "")
   *
   * Returned Parameters:
   * \li \b training_time (flexible_type: float). The training time of the algorithm in seconds
   * excluding all other preprocessing stages.
   *
   * \li \b __graph__ (unity_graph). The graph object with the field "distance"
   * on each vertex. The "distance" field (float) corresponds to the
   * distance of the vertex from the source_vid. If the vertex was unreachable,
   * it has weight infinity.
   */
    std::vector<toolkit_function_specification> get_toolkit_function_registration();

  } // namespace sssp
} // namespace turi
#endif
