/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_QUERY_EVAL_SORT_AND_MERGE_HPP
#define TURI_QUERY_EVAL_SORT_AND_MERGE_HPP

namespace turi {
namespace query_eval {

#include <memory>
/**
 * \ingroup sframe_query_engine
 * \addtogroup Algorithms Algorithms
 * \{
 */

/**
 * \internal
 * The merge stage of \ref sort.
 *
 * The input is a partially sorted(partitioned) sframe, represented by
 * an sarray<string> with N segments. Each segment
 * is a partitioned key range, and segments are ordered by
 * the key orders.
 *
 * Given the partially sorted sframe, this function will in parallel
 * sort each partition, and concat the result into final sframe.
 *
 * \param partition_array the serialized input sframe, partially sorted
 * \param partition_sorted flag whether the partition is already sorted
 * \param partition_sizes the estimate size of each partition
 * \param sort_orders sort order of the keys
 * \param permute_order The output order of the keys. column {permute_order[i]}
 * will be stored in column i of the final SFrame
 * \param column_names column names of the final sframe
 * \param column_types column types of the final sframe
 *
 * \return a sorted sframe.
 */
std::shared_ptr<sframe> sort_and_merge(
    const std::shared_ptr<sarray<std::pair<flex_list, std::string>>>& partition_array,
    const std::vector<bool>& partition_sorted,
    const std::vector<size_t>& partition_sizes,
    const std::vector<bool>& sort_orders,
    const std::vector<size_t>& permute_order,
    const std::vector<std::string>& column_names,
    const std::vector<flex_type_enum>& column_types);

/// \}
} // enfd of query_eval
} // end of turicreate
#endif
