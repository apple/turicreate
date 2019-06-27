/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_GROUPBY_AGGREGATE_HPP
#define TURI_SFRAME_GROUPBY_AGGREGATE_HPP

#include <memory>
#include <vector>
#include <cstdint>
#include <functional>
#include <unordered_set>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <core/storage/sframe_data/group_aggregate_value.hpp>

namespace turi {

const std::set<std::string> registered_arg_functions = {"argmax","argmin"};


/**
 * \ingroup sframe_physical
 * \addtogroup groupby_aggregate Groupby Aggregation
 * \{
 */

/**
 * Groupby Aggregate function for an SFrame.
 * Given the source SFrame this function performs a group-by aggregate of the
 * SFrame, using one or more columns to define the group key, and a descriptor
 * for how to aggregate other non-key columns.
 *
 * For instance given an SFrame:
 * \verbatim
 * user_id  movie_id  rating  time
 *      5        10       1    4pm
 *      5        15       2    1pm
 *      6        12       1    2pm
 *      7        13       1    3am
 * \endverbatim
 * \code
 * sframe output = turi::groupby_aggregate(input,
 *                    {"user_id"},
 *                    {"movie_count", "rating_sum"},
 *                    {{"movie_id", std::make_shared<groupby_operators::count>()},
 *                    {"rating", std::make_shared<groupby_operators::sum>()}});
 * \endcode
 *
 * will generate groups based on the user_id column, and within each group,
 * count the movie_id, and sum the ratings.
 * \verbatim
 * user_id  "Count of movie_id"  "Sum of rating"
 *      5                    2               3
 *      6                    1               1
 *      7                    1               1
 * \endverbatim
 *
 * See groupby_aggregate_operators for operators that have been implemented.
 *
 * Describing a Group
 * ------------------
 * A group is basically a pair of column-name and the operator.
 * The column name can be any existing column in the table (there is no
 * restriction. You can group on user_id and aggregate on user_id, though the
 * result is typically not very meaningful). A special column name with the
 * empty string "" is also defined in which case, the aggregator will be
 * sent a flexible type of type FLEX_UNDEFINED for every row (this is useful
 * for COUNT).
 *
 * \param source The input SFrame to group
 * \param keys An array of column names to generate the group on
 * \param group_output_columns The output column names for each aggregate.
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
 * \param max_buffer_size The maximum size of intermediate aggregation buffers
 *
 * \return The new aggregated SFrame. throws a string exception on failures.
 */
sframe groupby_aggregate(const sframe& source,
      const std::vector<std::string>& keys,
      const std::vector<std::string>& group_output_columns,
      const std::vector<std::pair<std::vector<std::string>,
                                  std::shared_ptr<group_aggregate_value>>>& groups,
      size_t max_buffer_size = SFRAME_GROUPBY_BUFFER_NUM_ROWS);


/// \}
} // end of turi
#endif //TURI_SFRAME_GROUPBY_AGGREGATE_HPP
