/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_QUERY_EVAL_SORT_HPP
#define TURI_QUERY_EVAL_SORT_HPP

#include <vector>
#include <memory>

namespace turi {

class sframe;

namespace query_eval {

struct planner_node;

/**
 * \ingroup sframe_query_engine
 * \addtogroup Algorithms Algorithms
 * \{
 */

/**
 * Sort given SFrame.
 *
 * The algorithm is like the following:
 *   - First do a quantile sketch over all sort columns and use the quantile sketch to
 *     figure out the partition keys that we will use to split the sframe rows into
 *     small chunks so that each chunk is realtively sorted. Each chunk is small enough
 *     so that we could sort in memory
 *   - Scatter partition the sframe according to above partition keys. The resulting
 *     value is persisted. Each partition is stored as one segment in a sarray.
 *   - The sorting resulting is then lazily materialized through le_sort operator
 *
 * There are a few optimizations along the way:
 *   - if all sorting keys are the same, then no need to sort
 *   - if the sframe is small enough to fit into memory, then we simply do a in
 *     memory sort
 *   - if some partitions of the sframe have the same sorting key, then that partition
 *     will not be sorted
 *
 * Also see \ref ec_sort for another sort implementation
 *
 * \param sframe_planner_node The lazy sframe to be sorted
 * \param sort_column_names The columns to be sorted
 * \param sort_orders The order for each column to be sorted, true is ascending
 * \return The sorted sframe
 **/
std::shared_ptr<sframe> sort(
    std::shared_ptr<planner_node> sframe_planner_node,
    const std::vector<std::string> column_names,
    const std::vector<size_t>& sort_column_indices,
    const std::vector<bool>& sort_orders);

} // end of query_eval
} // end of turicreate

/// \}

#endif //TURI_SFRAME_SORT_HPP
