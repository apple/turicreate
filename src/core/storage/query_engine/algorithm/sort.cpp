/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <functional>
#include <algorithm>
#include <core/util/dense_bitset.hpp>
#include <core/parallel/thread_pool.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <core/random/random.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_config.hpp>
#include <ml/sketches/quantile_sketch.hpp>
#include <ml/sketches/streaming_quantile_sketch.hpp>
#include <core/storage/query_engine/planning/planner_node.hpp>
#include <core/storage/query_engine/planning/planner.hpp>
#include <core/storage/query_engine/operators/project.hpp>
#include <core/storage/query_engine/operators/union.hpp>
#include <core/storage/query_engine/algorithm/sort_and_merge.hpp>
#include <core/storage/query_engine/algorithm/sort_comparator.hpp>

namespace turi {

namespace query_eval {

// heuristic
// guestimate for the size of each cell
// and the memory overhead of each row
constexpr size_t CELL_SIZE_ESTIMATE = 64;
constexpr size_t ROW_SIZE_ESTIMATE = 32;

/**
 * Create a quantile sketch for the key columns so that we can decide how to partition
 * the sframe
 */
static
std::shared_ptr<sketches::streaming_quantile_sketch<flexible_type, less_than_full_function>>
create_quantile_sketch(std::shared_ptr<planner_node>&  sframe_planner_node,
                       const std::vector<bool>&  sort_orders ) {

  auto comparator =  less_than_full_function(sort_orders);
  turi::mutex lock;
  size_t num_threads = thread::cpu_count();
  size_t num_rows = infer_planner_node_length(sframe_planner_node);
  ASSERT_TRUE(num_rows != (size_t)(-1));
  size_t num_to_sample = std::min<size_t>(SFRAME_SORT_PIVOT_ESTIMATION_SAMPLE_SIZE,
                                          num_rows);
  float sample_ratio = (float)num_to_sample / num_rows;
  turi::atomic<size_t> num_sampled = 0;

  typedef sketches::streaming_quantile_sketch<flexible_type, less_than_full_function> sketch_type;
  sketch_type global_quantiles(0.005, comparator);
  std::vector<sketch_type> local_sketch_vector;
  for (size_t i = 0; i < num_threads; ++i) {
    local_sketch_vector.push_back(sketch_type(0.005, comparator));
  }

  auto sample_and_add_to_sketch_callback = [&](size_t segment_id,
                                               const std::shared_ptr<sframe_rows>& data) {
    auto& local_sketch = local_sketch_vector[segment_id];
    for (const auto& row: (*data)) {
      if (num_sampled == num_to_sample) {
        return true;
      }
      if (turi::random::fast_bernoulli(sample_ratio)) {
        local_sketch.add(row);
        ++num_sampled;
      }
    }
    return false;
  };

  logstream(LOG_INFO) << "Sampling pivot proportion: " << sample_ratio << std::endl;
  planner().materialize(sframe_planner_node,
                        sample_and_add_to_sketch_callback,
                        num_threads);
  for (auto& local_sketch: local_sketch_vector) {
    local_sketch.substream_finalize();
    global_quantiles.combine(local_sketch);
  }
  global_quantiles.combine_finalize();
  return std::make_shared<sketch_type>(global_quantiles);
}

/**
 * Find the "spliting points" that can partition the sframe into roughly similar
 * size chunks so that elements between chunks are relatively ordered.
 *
 * The way to do this is to do a sketch summary over the sorted columns, find the
 * quantile keys for each incremental quantile and use that key as "spliting point".
 *
 * \param sframe_ptr The lazy sframe that needs to be sorted
 * \param sort_orders The sort order for the each sorted columns, true means ascending
 * \param num_partitions The number of partitions to partition the result to
 * \param[out] partition_keys The "pivot point". There will be num_partitions-1 of these.
 * \param[out] partition_sorted Indicates whether or not a given partition contains
 *   all the same key hence no need to sort later
 * \return true if all key values are the same(hence no need to sort), false otherwise
 **/
bool get_partition_keys(
  std::shared_ptr<planner_node>   sframe_planner_node,
  const std::vector<bool>&        sort_orders,
  size_t                          num_partitions,
  std::vector<flexible_type>&     partition_keys) {

  auto quantiles = create_quantile_sketch(sframe_planner_node, sort_orders);

  // figure out all the cutting place we need for the each partion by calculating
  // quantiles
  double quantile_unit = 1.0 / num_partitions;
  flexible_type quantile_val;

  for (size_t i = 0;i < num_partitions - 1; ++i) {
    quantile_val = quantiles->query_quantile((i + 1) * quantile_unit);
    partition_keys.push_back(quantile_val);
  }
  return false;
}

/**
 * Partition given sframe into multiple partitions according to given partition key.
 * This results to multiple partitions and partitions are relatively ordered.
 *
 * This function writes the resulting partitions into a sarray<string> type, where
 * each segment in the sarray is one partition that are relatively ordered.
 *
 * We store a serialized version of original sframe sorting key columns and values
 * \param sframe_ptr The lazy sframe to be scatter partitioned
 * The key columns must be the lowest numbered columns.
 * \param num_sort_columns Columns [0, num_sort_columns - 1] are the key
 * columns.
 * \param sort_orders The ascending/descending order for each sorting column.
 * sort_orders.size() == num_sort_columns.
 * \param partition_keys The "spliting" point to partition the sframe
 * \param partition_sizes The estimated size of each sorted partition
 * \param partition_sorted Flag of weather each partition is sorted
 *
 * \return a pointer to a persisted sarray object, the sarray stores serialized
 *   values of partitioned sframe, with values between segments relatively ordered.
 *   Each row of the returned SArray is a pair<flex_list, string>
 *   where the first element of the pair is the key and the 2nd element of the
 *   pair is the serialized values.
**/
static std::shared_ptr<sarray<std::pair<flex_list, std::string> >> scatter_partition(
  const std::shared_ptr<planner_node> sframe_planner_node,
  size_t num_sort_columns,
  const std::vector<bool>& sort_orders,
  const std::vector<flexible_type>& partition_keys,
  std::vector<size_t>& partition_sizes,
  dense_bitset& partition_sorted) {

  log_func_entry();

  size_t num_partitions_keys = partition_keys.size() + 1;
  logstream(LOG_INFO) << "Scatter partition for sort, scatter to " +
        std::to_string(num_partitions_keys) + " partitions" << std::endl;

  // Preparing resulting sarray for writing
  auto parted_array = std::make_shared<sarray<std::pair<flex_list, std::string>>>();
  parted_array->open_for_write(num_partitions_keys);

  std::vector<sarray<std::pair<flex_list, std::string>>::iterator> outiter_vector(num_partitions_keys);
  for(size_t i = 0; i < num_partitions_keys; ++i) {
    outiter_vector[i] = parted_array->get_output_iterator(i);
  }

  // Create a mutex for each partition
  std::vector<mutex> outiter_mutexes(num_partitions_keys);
  std::vector<simple_spinlock> sorted_mutexes(num_partitions_keys);
  std::vector<flex_list> first_sort_key(num_partitions_keys);
  std::vector<size_t> partition_size_in_bytes(num_partitions_keys, 0);
  std::vector<size_t> partition_size_in_rows(num_partitions_keys, 0);

  // Iterate over each row of the given SFrame, compare against the partition key,
  // and write that row to the appropriate segment of the partitioned sframe_ptr
  size_t num_threads = thread::cpu_count();
  less_than_full_function less_than(sort_orders);

  // thread local buffers
  std::vector<std::vector<flexible_type>>
      sort_keys_buffers(thread::cpu_count(), std::vector<flexible_type>(num_sort_columns));
  std::vector<std::string> arcout_buffers(thread::cpu_count());
  std::vector<oarchive> oarc_buffers(thread::cpu_count());
  auto partial_sort_callback = [&](size_t segment_id,
                                   const std::shared_ptr<sframe_rows>& data) {
    oarchive& oarc = oarc_buffers[thread::thread_id()];
    std::vector<flexible_type>& sort_keys = sort_keys_buffers[thread::thread_id()];
    for(const auto& item: (*data)) {
      // extract sort key
      for(size_t i = 0; i < num_sort_columns; i++) {
        sort_keys[i] = item[i];
      }

      // find which partition the value belongs to
      size_t partition_id = num_partitions_keys - 1;
      partition_id = std::distance(partition_keys.begin(),
           std::lower_bound(partition_keys.begin(),
                            partition_keys.end(),
                            sort_keys,
                            [&](const flexible_type& left, const std::vector<flexible_type>& right) {
                              return less_than(left.get<flex_list>(), right);
                            }
                            ));
      // std::lower_bound returns the first element that is >= the sort_key
      // On the other hand for the partition number, I need the last element that is <= the sort key
      // So sometimes I need to decrement by one
      // if partition_id is past the end, decrement by 1
      // if sort_key < partition, decrement partition id
      if (partition_id == partition_keys.size() ||
          (partition_id > 0 && less_than(sort_keys, partition_keys[partition_id].get<flex_list>()))) {
        --partition_id;
      }
      DASSERT_TRUE(partition_id < num_partitions_keys);

      // double checked locking optimization on partition sorted
      if(partition_sorted.get(partition_id)) {
        sorted_mutexes[partition_id].lock();
        if(partition_sorted.get(partition_id)) {
          if(first_sort_key[partition_id].size() == 0) {
            first_sort_key[partition_id] = sort_keys;
          } else {
            if(first_sort_key[partition_id] != sort_keys) {
              partition_sorted.set(partition_id, false);
            }
          }
        }
        sorted_mutexes[partition_id].unlock();
      }

      // stream the key and value to output segment
      oarc.off = 0;
      for (size_t i = num_sort_columns; i < item.size(); ++i) oarc << item[i];
      std::string& arcout = arcout_buffers[thread::thread_id()];
      arcout.assign(oarc.buf, oarc.off);

      // write to coresponding output segment
      outiter_mutexes[partition_id].lock();

      // Calculate roughly how much memory each partition will take up when
      // loaded to be sorted
      // say that each row adds 32 bytes and each cell adds 64 bytes
      partition_size_in_bytes[partition_id] +=
          oarc.off + (num_sort_columns * CELL_SIZE_ESTIMATE) + ROW_SIZE_ESTIMATE;
      ++partition_size_in_rows[partition_id];

      *(outiter_vector[partition_id]) = {sort_keys, arcout};
      ++outiter_vector[partition_id];

      outiter_mutexes[partition_id].unlock();
    }
    return false;
  };

  planner().materialize(sframe_planner_node, partial_sort_callback, num_threads);
  for (auto& oarc: oarc_buffers) free(oarc.buf);
  parted_array->close();


  for(size_t i = 0; i < num_partitions_keys; ++i) {
    logstream(LOG_INFO) << "Size of partition " << i << ": " <<
      partition_size_in_bytes[i] << std::endl;
  }

  partition_sizes = partition_size_in_bytes;

  return parted_array;
}

/**
 * Sort the whole sframe in memory.
 * This is used in case the sframe is small and we can sort in memory
 */
std::shared_ptr<sframe> sort_sframe_in_memory(
  std::shared_ptr<planner_node> sframe_planner_node,
  const std::vector<std::string>& column_names,
  const std::vector<size_t>& sort_columns,
  const std::vector<bool>& sort_orders) {

  auto column_types = infer_planner_node_type(sframe_planner_node);

  auto sf = planner().materialize(sframe_planner_node);
  std::vector<std::vector<flexible_type>> rows;
  sf.get_reader()->read_rows(0, sf.size(), rows);

  less_than_partial_function comparator(sort_columns, sort_orders);
  std::sort(rows.begin(), rows.end(), comparator);

  auto ret = std::make_shared<sframe>();
  ret->open_for_write(column_names, column_types, "", 1);
  std::move(rows.begin(), rows.end(), ret->get_output_iterator(0));
  ret->close();
  return ret;
}

/**
 * Main implementation of the top level sort API.
 */
std::shared_ptr<sframe> sort(
    std::shared_ptr<planner_node> sframe_planner_node,
    const std::vector<std::string> column_names,
    const std::vector<size_t>& sort_column_indices,
    const std::vector<bool>& sort_orders) {
  log_func_entry();

  auto column_types = infer_planner_node_type(sframe_planner_node);

  /*
   * We partition the frame into 2 sets
   * The sort columns (listed by sort_column_indices, and the node key_column)
   * The value columns (listed by value_column_indices, and the node value_columns)
   */
  size_t num_rows = infer_planner_node_length(sframe_planner_node);
  size_t num_columns = column_types.size();
  std::set<size_t> sort_column_indices_set(sort_column_indices.begin(),
                                           sort_column_indices.end());

  std::vector<size_t> value_column_indices;
  for (size_t i = 0;i < num_columns ; ++i) {
    if (sort_column_indices_set.count(i) == 0) value_column_indices.push_back(i);
  }
  auto key_columns = op_project::make_planner_node(sframe_planner_node, sort_column_indices);

  // now. Annoyingly enough, I can't project an empty column.
  std::shared_ptr<planner_node> value_columns;
  if (!value_column_indices.empty()) {
    value_columns = op_project::make_planner_node(sframe_planner_node, value_column_indices);
  }
  // If the length of the sframe node is unknown,
  // materialize the key columns to get the length
  if (num_rows == (size_t)(-1)) {
    query_eval::planner().materialize(key_columns);
    num_rows = infer_planner_node_length(key_columns);
  }

  // get sort column indices from column names and also check column types
  std::vector<flex_type_enum> supported_types =
      {flex_type_enum::STRING, flex_type_enum::INTEGER, flex_type_enum::FLOAT,flex_type_enum::DATETIME};
  std::set<flex_type_enum> supported_type_set(supported_types.begin(), supported_types.end());

  for(auto column_index: sort_column_indices) {
    auto col_type = column_types[column_index];
    if (supported_type_set.count(col_type) == 0) {
      auto msg = std::string("Only column with type 'int', 'float', 'string', and 'datetime' can be sorted. Found column type: ") + flex_type_enum_to_name(col_type);
      log_and_throw(msg);
    }
  }

  // TODO: Estimate the size of the sframe so that we could decide number of
  // chunks. To account for strings, we estimate each cell is 64 bytes.
  // I'd love to estimate better.
  size_t estimated_sframe_size = num_rows * num_columns * CELL_SIZE_ESTIMATE+ num_rows * ROW_SIZE_ESTIMATE;
  size_t num_partitions = std::ceil((1.0 * estimated_sframe_size) / sframe_config::SFRAME_SORT_BUFFER_SIZE);

  // Make partitions small enough for each thread to (theoretically) sort at once
  num_partitions = num_partitions * thread::cpu_count();

  // If we have more partitions than this, we could run into open file
  // descriptor limits. num_partitions can be 0 if frame has 0 rows.
  num_partitions = std::min<size_t>(num_partitions, SFRAME_SORT_MAX_SEGMENTS);

  // Shortcut -- if only one partition, do a in memory sort and we are done
  if (num_partitions <= thread::cpu_count()) {
    logstream(LOG_INFO) << "Sorting SFrame in memory" << std::endl;
    auto ret = sort_sframe_in_memory(sframe_planner_node,
                                     column_names,
                                     sort_column_indices,
                                     sort_orders);
    return ret;
  }

  // This is a collection of partition keys sorted in the required order.
  // Each key is a flex_list value that contains the spliting value for
  // each sort column. Together they defines the "cut line" for all rows in
  // the SFrame.
  std::vector<flexible_type> partition_keys;

  // Do a quantile sketch on the sort columns to figure out the "splitting" points
  // for the SFrame
  timer ti;
  bool all_sorted = get_partition_keys (
    key_columns,
    sort_orders, num_partitions, // in parameters
    partition_keys);  // out parameters
  logstream(LOG_INFO) << "Pivot estimation step: " << ti.current_time() << std::endl;

  // In rare case all values in the SFrame are the same, so no need to sort
  if (all_sorted)  {
    auto ret = planner().materialize(sframe_planner_node);
    return std::make_shared<sframe>(ret);
  }

  // scatter partition the sframe into multiple chunks, chunks are relatively
  // sorted, but each chunk is not sorted. The sorting of each chunk is delayed
  // until it is consumed. Each chunk is stored as one segment for a sarray.
  // The chunk stores a serailized version of key and value
  std::vector<size_t> partition_sizes;

  // In the case where all sort keys in a given partition are the same, then
  // there is no need to sort the partition. This information is derived from
  // scattering
  dense_bitset partition_sorted(num_partitions);
  partition_sorted.fill();

  // We perform the scatter.
  // rebuild the sframe so that the key columns are the lowest column indices

  std::shared_ptr<planner_node> key_and_value_columns;
  if (value_columns) {
    // reproject the key columns. It is more efficient to keep the query plan
    // symmetric here. For instance if the original plan has a "filter"
    // this will result in a union of a "filter" and a fully materialized column
    // which cannot be parallelized effectively making the scatter operation
    // fuly single threaded.
    // By keeping the original query plan; but just reprojecting the columns,
    // we keep the properties of the original plan. i.e. if filter
    // is the last stage, it will still be nice and parallel.
    key_columns = op_project::make_planner_node(sframe_planner_node, sort_column_indices);
    key_and_value_columns = op_union::make_planner_node(key_columns, value_columns);
  } else {
    key_and_value_columns = key_columns;
  }
  ti.start();
  auto partition_array = scatter_partition(
    key_and_value_columns,
    sort_orders.size(),
    sort_orders,
    partition_keys, partition_sizes, partition_sorted);
  logstream(LOG_INFO) << "Scatter step: " << ti.current_time() << std::endl;

  ti.start();
  // the partition process reorganizes the sframe so that all the key columns
  // come first, than all the value columns. But when we write it out, we want
  // it back in the original ordering.
  // permute_ordering[i] is the reverse permutation. i.e.
  // column {permute_ordering[i]} is going to be placed in column i.
  std::vector<size_t> permute_ordering(num_columns);
  size_t value_column_counter = 0;
  for (size_t i = 0;i < num_columns; ++i) {
    if (sort_column_indices_set.count(i) == 0) {
      // its a value column. In the intermediate SFrame, all value columns
      // come after the key columns and in the same original sequence.
      // hence the ordering is simply the number of key columns, plus the
      // number of value columns seen so far.
      permute_ordering[i]  = sort_column_indices.size() + value_column_counter;
      ++value_column_counter;
    }
  }
  // key columns are different.
  // In the intermediate SFrame, all key columns
  // come first in the order defined by sort_column_indices.
  for (size_t i = 0;i < sort_column_indices.size(); ++i) {
    permute_ordering[sort_column_indices[i]] = i;
  }

  std::vector<bool> partition_sorted_vec_bool(partition_sorted.size());
  for (size_t i = 0;i < partition_sorted.size(); ++i) {
    partition_sorted_vec_bool[i] = partition_sorted.get(i);
  }
  auto ret = sort_and_merge(
    partition_array,
    partition_sorted_vec_bool,
    partition_sizes,
    sort_orders,
    permute_ordering,
    column_names,
    column_types);
  logstream(LOG_INFO) << "Sort and merge step: " << ti.current_time() << std::endl;

  return ret;
}


} // namespace query_eval
} // namespace turi
