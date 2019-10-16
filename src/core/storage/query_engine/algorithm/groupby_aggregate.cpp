/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <vector>
#include <core/logging/logger.hpp>
#include <core/system/platform/timer//timer.hpp>
#include <core/storage/query_engine/planning/planner_node.hpp>
#include <core/storage/query_engine/planning/planner.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/storage/query_engine/operators/project.hpp>
#include <core/storage/query_engine/algorithm/groupby_aggregate.hpp>
#include <core/storage/sframe_data/group_aggregate_value.hpp>
#include <core/storage/sframe_data/groupby_aggregate_impl.hpp>
#include <core/storage/sframe_data/sframe_config.hpp>
#include <core/storage/sframe_data/groupby_aggregate.hpp>

namespace turi {
namespace query_eval {

std::shared_ptr<sframe>
    groupby_aggregate(
      const std::shared_ptr<planner_node>& source,
      const std::vector<std::string>& source_column_names,
      const std::vector<std::string>& keys,
      const std::vector<std::string>& output_column_names,
      const std::vector<std::pair<std::vector<std::string>,
                                  std::shared_ptr<group_aggregate_value>>>& groups) {
  // first, sanity checks
  // check that group keys exist
  if (output_column_names.size() != groups.size()) {
    log_and_throw("There must be as many output columns as there are groups");
  }
  {
    // check that output column names are all unique, and do not intersect with
    // keys. Since empty values will be automatically assigned, we will skip
    // those.
    std::set<std::string> all_output_columns(keys.begin(), keys.end());
    size_t named_column_count = 0;
    for (auto s: output_column_names) {
      if (!s.empty()) {
        all_output_columns.insert(s);
        ++named_column_count;
      }
    }
    if (all_output_columns.size() != keys.size() + named_column_count) {
      log_and_throw("Output columns names are not unique");
    }
  }

  std::map<std::string, size_t> source_column_to_index;
  for (size_t i = 0;i < source_column_names.size(); ++i) {
    source_column_to_index[source_column_names[i]] = i;
  }
  auto source_types = infer_planner_node_type(source);
  ASSERT_EQ(source_column_names.size(), source_column_to_index.size());
  ASSERT_EQ(source_types.size(), source_column_names.size());

  for (const auto& key: keys) {
    // check that the column name is valid
    if (!source_column_to_index.count(key)) {
      log_and_throw("SFrame does not contain column " + key);
    }
  }

  // check that each group is valid
  for (const auto& group: groups) {
    // check that the column name is valid
    if (group.first.size() > 0) {
      for(unsigned int index = 0; index < group.first.size(); index++) {
        auto& col_name = group.first[index];
        if (!source_column_to_index.count(col_name)) {
          log_and_throw("SFrame does not contain column " + col_name);
        }
        if(turi::registered_arg_functions.count(group.second->name()) != 0 && index > 0)
          continue;
        // check that the types are valid
        size_t column_number = source_column_to_index.at(col_name);

        if (!group.second->support_type(source_types[column_number])) {
          log_and_throw("Requested operation: " + group.second->name() +
                        " not supported on the type of column " + col_name);
        }
      }
    }
  }

  // key should not have repeated columns
  std::set<std::string> key_columns;
  std::set<std::string> group_columns;
  for (const auto& key: keys) key_columns.insert(key);
  for (const auto& group: groups) {
    for(auto& col_name : group.first) {
      group_columns.insert(col_name);
    }
  }
  if (key_columns.size() != keys.size()) {
      log_and_throw("Group by key cannot have repeated column names");
  }

  // ok. select out just the columns I care about
  // begin with the key columns
  std::vector<std::string> relevant_column_names(key_columns.begin(), key_columns.end());
  // then all the group columns (as long as they are not also key columns)
  for (const auto& group_column: group_columns) {
    if (group_column != "" && key_columns.count(group_column) == 0) {
      relevant_column_names.push_back(group_column);
    }
  }
  // column name to column number of the frame_with_relevant_cols
  std::map<std::string, size_t> relevant_column_to_index;
  // which columns from source SFrame to project over to this SFrame
  std::vector<size_t> relevant_source_indices(relevant_column_names.size());
  for (size_t i = 0;i < relevant_column_names.size(); ++i) {
    relevant_source_indices[i] = source_column_to_index.at(relevant_column_names[i]);
    relevant_column_to_index[relevant_column_names[i]] = i;
  }
  auto frame_with_relevant_cols = op_project::make_planner_node(source,
                                                                relevant_source_indices);


  // prepare the output frame
  auto output = std::make_shared<sframe>();;
  std::vector<std::string> column_names;
  std::vector<flex_type_enum> column_types;
  // output frame has the key column name and types
  for (const auto& key: key_columns) {
    column_names.push_back(key);
    column_types.push_back(source_types.at(source_column_to_index.at(key)));
  }

  // then for each group, make a unique name and determine the output group type
  for (size_t i = 0;i < groups.size(); ++i) {
    const auto& group = groups[i];
    std::string candidate_name = output_column_names[i];
    if (candidate_name.empty()) {

      std::string root_candidate_name;
      if(turi::registered_arg_functions.count(group.second->name()) == 0) {

        for (auto& col_name: group.first) {
          if (root_candidate_name.empty()) {
            root_candidate_name += " of " + col_name;
          } else {
            root_candidate_name += "_" + col_name;
          }
        }

        root_candidate_name = group.second->name() + root_candidate_name;
      } else {
        if(group.first.size() != 2)
          log_and_throw("arg functions takes exactly two arguments");
        root_candidate_name += group.first[1] + " for " + group.second->name() + " of " + group.first[0];
      }
      candidate_name = root_candidate_name;
      size_t ctr = 1;
      // keep trying to come up with a unique column name
      while (std::find(column_names.begin(),
                       column_names.end(),
                       candidate_name) != column_names.end()) {
        candidate_name = root_candidate_name + "." + std::to_string(ctr);
        ++ctr;
      }
    }
    column_names.push_back(candidate_name);

    std::vector<flex_type_enum> input_types;
    for(auto col_name : group.first) {
      input_types.push_back(source_types.at(source_column_to_index.at(col_name)));
    }
    auto output_type = group.second->set_input_types(input_types);
    column_types.push_back(output_type);
  }

  size_t nsegments = thread::cpu_count() * std::max<size_t>(1, log2(thread::cpu_count()));

  output->open_for_write(column_names,
                         column_types,
                         "",
                         nsegments);


  groupby_aggregate_impl::group_aggregate_container
      container(SFRAME_GROUPBY_BUFFER_NUM_ROWS, nsegments);

  // ok the input sframe (frame_with_relevant_cols) contains all the values
  // we care about. However, the challenge here is to figure out how the keys
  // and values line up. By construction, all the key columns come first.
  // which is good. But group columns can be pretty much anywhere.
  size_t num_keys = keys.size();
  for (const auto& group: groups) {
    std::vector<size_t> column_numbers;
    for(auto& col_name : group.first) {
      column_numbers.push_back(relevant_column_to_index.at(col_name));
    }

    container.define_group(column_numbers, group.second);
  }
  // done. now we can begin parallel processing

  // shuffle the rows based on the value of the key column.
  logstream(LOG_INFO) << "Filling group container: " << std::endl;
  timer ti;
  planner().materialize(frame_with_relevant_cols,
                        [&](size_t segmentid,
                            const std::shared_ptr<sframe_rows>& rows)->bool {
                          container.init_tls();
                          if (rows == nullptr) return true;
                          for (auto& row: *rows) {
                            container.add(row, num_keys);
                          }
                          container.flush_tls();
                          return false;
                        },
                        thread::cpu_count());

  logstream(LOG_INFO) << "Group container filled in " << ti.current_time() << std::endl;
  logstream(LOG_INFO) << "Writing output: " << std::endl;
  ti.start();
  container.group_and_write(*output);
  logstream(LOG_INFO) << "Output written in: " << ti.current_time() << std::endl;
  output->close();
  return output;
}
} // query_eval
} // end of turicreate
