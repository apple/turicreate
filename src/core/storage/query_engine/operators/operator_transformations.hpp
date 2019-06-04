/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_OPERATOR_TRANSFORMATIONS_H_
#define TURI_SFRAME_QUERY_OPERATOR_TRANSFORMATIONS_H_

#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>

namespace turi { namespace query_eval {

/** Turns a node graph into the proper segmented part.
 */
pnode_ptr make_segmented_graph(pnode_ptr n, size_t split_idx, size_t n_splits,
    std::map<pnode_ptr, pnode_ptr>& memo);

/** Slice the node graph input with begin and end.
 *
 * Note:
 * 1. only allows forward slice, i.e begin_index <= end_index
 * 2. allows recursive slice, for example:
 *
 * \code
 * n1 = make_sliced_graph(n0, 5, 10) // n1 contains row 5 to 9 of n0
 * n2 = make_sliced_graph(n1, 1, 2) // n2 contains row 1 of n1, which is row 6 of n0
 * \endcode
 *
 * 3. final slice range cannot exeeds the original graph
 *
 * \code
 * n1 = make_sliced_graph(n0, 0, n0.size() +1) // throws error
 * \endcode
 */
pnode_ptr make_sliced_graph(pnode_ptr n, size_t begin_index, size_t end_index,
                            std::map<pnode_ptr, pnode_ptr>& memo);
}}

#endif /* _OPERATOR_TRANSFORMATIONS_H_ */
