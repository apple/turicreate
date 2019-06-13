/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <core/system/platform/timer//timer.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <core/storage/sframe_data/sframe_config.hpp>
#include <core/storage/sframe_data/sarray_v2_block_manager.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/query_engine/operators/project.hpp>
#include <core/storage/query_engine/operators/union.hpp>
#include <core/storage/query_engine/operators/sframe_source.hpp>
#include <core/storage/query_engine/operators/range.hpp>
#include <core/storage/query_engine/algorithm/sort.hpp>
#include <core/storage/query_engine/algorithm/ec_permute.hpp>
#include <core/storage/query_engine/planning/planner.hpp>
namespace turi {
namespace query_eval {
using sframe_config::SFRAME_SORT_BUFFER_SIZE;
std::shared_ptr<sframe> ec_sort(
    std::shared_ptr<planner_node> sframe_planner_node,
    const std::vector<std::string> column_names,
    const std::vector<size_t>& key_column_indices,
    const std::vector<bool>& sort_orders) {

  // prep some standard metadata
  //  - num_columns
  //  - num_rows
  //  - key column information
  //      - key_column_names
  //      - key_column_indices
  //      - key_column_indices_set
  //      - key_columns
  //      - num_key_columns
  //  - value column information
  //      - value_column_names
  //      - value_column_indices
  //      - value_columns
  //      - value_column_types
  //
  size_t num_columns = column_names.size();
  int64_t num_rows = infer_planner_node_length(sframe_planner_node);
  if (num_rows == -1) {
    planner().materialize(sframe_planner_node);
    num_rows = infer_planner_node_length(sframe_planner_node);
  }
  ASSERT_GE(num_rows, 0);
  // fast path for few number of rows.
  // fast path for no value columns
  if (num_rows < 1000 ||
      key_column_indices.size() == column_names.size()) {
    return sort(sframe_planner_node, column_names,
                key_column_indices, sort_orders);
  }

  // key columns
  auto key_columns = op_project::make_planner_node(sframe_planner_node, key_column_indices);
  std::vector<std::string> key_column_names;
  std::for_each(key_column_indices.begin(), key_column_indices.end(),
                 [&](size_t i) {
                   key_column_names.push_back(column_names[i]);
                 });
  std::set<size_t> key_column_indices_set(key_column_indices.begin(),
                                          key_column_indices.end());
  size_t num_key_columns = key_column_indices.size();

  // value columns
  std::vector<std::string> value_column_names;
  std::vector<size_t> value_column_indices;
  for (size_t i = 0 ;i < num_columns; ++i) {
    if (key_column_indices_set.count(i) == 0) value_column_indices.push_back(i);
  }
  auto value_columns =
      op_project::make_planner_node(sframe_planner_node, value_column_indices);
  std::for_each(value_column_indices.begin(), value_column_indices.end(),
                 [&](size_t i) {
                   value_column_names.push_back(column_names[i]);
                 });
  std::vector<flex_type_enum> value_column_types = infer_planner_node_type(value_columns);
  {
    std::set<flex_type_enum> value_column_type_set(value_column_types.begin(),
                                                   value_column_types.end());
    // remove all the definitely small value types
    value_column_type_set.erase(flex_type_enum::INTEGER);
    value_column_type_set.erase(flex_type_enum::FLOAT);
    value_column_type_set.erase(flex_type_enum::DATETIME);
    // little heuristic. If all value columns are small and if there are
    // relatively few columns use the regular sort
    // TODO: yes 20 is a magic number.
    // On my Mac laptop this seems to roughly be the change over point.
    if (value_column_types.size() < 20 && value_column_type_set.size() == 0) {
      return sort(sframe_planner_node, column_names,
                  key_column_indices, sort_orders);
    }
  }

  // Forward Map Generation
  // ----------------------
  //
  // - A set of row numbers are added to the key columns, and the key
  // columns are sorted. And then dropped. This gives the inverse map.
  // (i.e. x[i] = j implies output row i is read from input row j)
  // - Row numbers are added again, and its sorted again by the first set
  // of row numbers. This gives the forward map (i.e. y[i] = j implies
  // input row i is written to output row j)
  // - (In SFrame pseudocode:
  //
  //     B = A[['key']].add_row_number('r1').sort('key')
  //     inverse_map = B['r1'] # we don't need this
  //     C = B.add_row_number('r2').sort('r1')
  //     foward_map = C['r2']

  std::shared_ptr<sarray<flexible_type> > forward_map;
  sframe sorted_key_columns;
  timer ti;
  ti.start();
  logstream(LOG_INFO) << "Creating forward map" << std::endl;

  {
    std::vector<std::string> forward_map_sort1_columns;
    // create new column names. Row number is first column. 'r1'
    forward_map_sort1_columns.push_back("r1");
    std::copy(key_column_names.begin(), key_column_names.end(),
              std::back_insert_iterator<std::vector<std::string>>(forward_map_sort1_columns));
    // all the key indices are all the columns
    std::vector<size_t> forward_map_sort1_column_indices;
    for (size_t i = 1; i <= key_column_names.size(); ++i) {
      forward_map_sort1_column_indices.push_back(i);
    }
    auto B = sort(op_union::make_planner_node(
                      op_range::make_planner_node(0, num_rows), key_columns),
                  forward_map_sort1_columns,
                  forward_map_sort1_column_indices,
                  sort_orders);
    logstream(LOG_INFO) << "sort finished in " << ti.current_time() << std::endl;
    ti.start();
    auto inverse_map = op_project::make_planner_node(
        op_sframe_source::make_planner_node(*B), {0});

    // remember the sorted column names. We are going to need it
    // when constructing the final SFrame
    sorted_key_columns = planner().materialize(
        op_project::make_planner_node(
            op_sframe_source::make_planner_node(*B),
            forward_map_sort1_column_indices));
    ASSERT_EQ(sorted_key_columns.num_columns(), num_key_columns);

    for (size_t i = 0;i < sorted_key_columns.num_columns(); ++i) {
      sorted_key_columns.set_column_name(i, key_column_names[i]);
    }

    // now generate the forward map
    ti.start();
    auto materialized_inverse_map = planner().materialize(inverse_map).select_column(0);
    sframe incremental_array = planner().materialize(op_range::make_planner_node(0, num_rows));

    forward_map = permute_sframe(incremental_array, materialized_inverse_map).select_column(0);
    logstream(LOG_INFO) << "forward map generation finished in " << ti.current_time() << std::endl;
  }


  // values_sframe: The raw sframe containing just the value columns
  sframe values_sframe = planner().materialize(value_columns);
  for (size_t i = 0;i < values_sframe.num_columns(); ++i) {
    values_sframe.set_column_name(i, value_column_names[i]);
  }
  // permute with the forward map
  sframe sorted_values_sframe = permute_sframe(values_sframe, forward_map);

  // generate the final sframe. combining the key and values sframes.
  // order the columns so that they are in the right order as the input.
  std::vector<std::shared_ptr<sarray<flexible_type>>> final_sframe_columns(num_columns);
  std::map<std::string, std::shared_ptr<sarray<flexible_type>>> final_name_to_column;
  for (size_t i = 0;i < sorted_key_columns.num_columns(); ++i) {
    final_name_to_column[key_column_names[i]] = sorted_key_columns.select_column(i);
  }
  for (size_t i = 0;i < sorted_values_sframe.num_columns(); ++i) {
    final_name_to_column[value_column_names[i]] = sorted_values_sframe.select_column(i);
  }

  for (size_t i = 0;i  < num_columns; ++i) {
    ASSERT_TRUE(final_name_to_column.count(column_names[i]) > 0);
    final_sframe_columns[i] = final_name_to_column[column_names[i]];
  }
  sframe final_sframe(final_sframe_columns, column_names);
  return std::make_shared<sframe>(final_sframe);
}

} // namespace query_eval
} // namespace turi
