/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ENGINE_HPP
#define TURI_SFRAME_QUERY_ENGINE_HPP
#include <vector>
#include <string>
#include <utility>
#include <memory>
#include <core/storage/sframe_data/group_aggregate_value.hpp>

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
 * Groupby aggregate algorithm that operates on lazy input.
 *
 * Identical to \ref turi::groupby_aggregate but can take a lazy input.
 *
 * \param source The lazy input node
 * \param source_column_names The column names of the input source
 * \param keys An array of column names to generate the group on
 * \param output_column_names The output column names for each aggregate.
 *                           This must be the same length as the 'groups' parameter.
 *                           Output column names must be unique and must not
 *                           share similar column names as keys. If there are any
 *                           empty entries, their values will be automatically
 *                           assigned.
 * \param groups A collection of {column_names, group operator} pairs describing
 *               the aggregates to generate. You can have multiple aggregators
 *               for each set of columns. You do not need every column in the source
 *               to be represented. This must be the same length as the
 *               'group_output_columns' parameter.

 */
std::shared_ptr<sframe> groupby_aggregate(
      const std::shared_ptr<planner_node>& source,
      const std::vector<std::string>& source_column_names,
      const std::vector<std::string>& keys,
      const std::vector<std::string>& output_column_names,
      const std::vector<std::pair<std::vector<std::string>,
                                  std::shared_ptr<group_aggregate_value>>>& groups);

/// \}
} // namespace query_eval
} // namespace turi
#endif
