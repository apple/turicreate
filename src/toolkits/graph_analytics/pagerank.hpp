/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_specification.hpp>
#ifndef TURI_UNITY_PAGERANK
#define TURI_UNITY_PAGERANK

namespace turi {
namespace pagerank {

/**
 * Obtains the registration for the PageRank Toolkit.
 *
 * Performs the following iterative pagerank computation:
 * \f[
 *   PR_i = r + (1.0 - r) \times \sum_{j\in\textrm{InNbrs}_i} \frac{PR_j}{\textrm{OutNbrs}_j}
 * \f]
 *
 * Note that this implements the 'unnormalized' pagerank value.
 *
 * <b> Toolkit Name: pagerank </b>
 *
 * Accepted Parameters:
 * \li \b threshold (flexible_type: float). The termination threshold. PageRank
 * values which change by less than this threshold will request neighbor
 * recomputation. Defaults to 1E-2. Must be > 0.
 *
 * \li \b random_jump_prob (flexible_type: float). The random jump probability.
 * Defaults to 0.15. Must be > 0 and < 1. Corresponds to r in the formula above.
 *
 * Returned Parameters:
 * \li \b training_time (flexible_type: float). The training time of the algorithm in seconds
 * excluding all other preprocessing stages.
 *
 * \li \b total_pagerank (flexible_type: float). The sum of all the pagerank
 * values on all vertices.
 *
 * \li \b delta (flexible_type: float). The absolute sum of all last changes
 * made to each vertex.
 *
 * \li \b __graph__ (unity_graph). The graph object with the fields "pagerank"
 * and "delta" on each vertex. The "pagerank" field (float) contains the
 * pagerank of the vertex, and the "delta" field (float) contains the last
 * change performed to the vertex's pagerank value.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace pagerank
} // namespace turi
#endif
