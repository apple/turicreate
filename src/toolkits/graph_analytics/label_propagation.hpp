/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_specification.hpp>
#ifndef TURI_UNITY_LABEL_PROPAGATION
#define TURI_UNITY_LABEL_PROPAGATION

namespace turi {
namespace label_propagation {

/**
 * Obtains the registration for the Label Propagation Toolkit.
 *
 * Performs the following iterative label propagation computation:
 * For class label k:
 * \f[
 *   PR_i[k] = PR_i[k] \times self\_weight + \sum_{j\in\textrm{InNbrs}_i} PR_j[k] * W_{j,i}
 *   PR_i[:] = Normalize(Pr_i[:])
 * \f]
 *
 * Reference:
 * Zhu, Xiaojin, and Zoubin Ghahramani. Learning from labeled and unlabeled
 * data with label propagation. Technical Report CMU-CALD-02-107, Carnegie
 * Mellon University, 2002.
 *
 * <b> Toolkit Name: label propagation</b>
 *
 * Accepted Parameters:
 * \li \b label_field (flexible_type: string). The vertex field for initial labels.
 *
 * \li \b threshold (flexible_type: float). The termination threshold in
 * average L2 norm.
 *
 * \li \b self_weight (flexible_type: float). The weight for self edge.
 *
 * \li \b weight_field (flexible_type: string). The edge field for edge weight.
 * If empty, then unit edge weight is used.
 *
 * \li \b undirected (flexible_type: int). If true, the label propagates
 * from both source to target, and target to source.
 *
 * Returned Parameters:
 *
 * \li \b training_time (flexible_type: float). The training time of the
 * algorithm in seconds excluding all other preprocessing stages.
 *
 * \li \b delta (flexible_type: float). The average of all last changes
 * made to each vertex in l2 norm.
 *
 * \li \b __graph__ (unity_graph). The graph object with the fields "P_0"
 * "P_1", ... "P_k" on each vertex, containing the probability
 * of the class for each vertex.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace pagerank
} // namespace turi
#endif
