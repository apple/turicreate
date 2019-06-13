/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include<memory>
#include<vector>
#include<core/storage/sframe_data/sarray.hpp>
#include<core/storage/sframe_data/sframe.hpp>
#include<core/storage/sframe_data/sframe_config.hpp>
#include<core/parallel/mutex.hpp>
#include<core/storage/query_engine/algorithm/sort_comparator.hpp>

namespace turi {
namespace query_eval {

/**
 * Gets the first row of a segment
 */
static size_t segment_start(
  std::unique_ptr<sarray_reader<std::pair<flex_list, std::string>>>& reader,
  size_t segmentid) {
  size_t ret = 0;
  for (size_t i = 0; i < segmentid; ++i)  ret += reader->segment_length(i);
  return ret;
}

static void read_one_chunk(
  std::unique_ptr<sarray_reader<std::pair<flex_list, std::string>>>& reader,
  size_t segment_id,
  size_t num_columns,
  std::vector<std::pair<flex_list, std::string>>& rows) {

  rows.resize(reader->segment_length(segment_id));
  rows.shrink_to_fit();
  size_t row_start = segment_start(reader, segment_id);
  size_t row_end = segment_start(reader, segment_id + 1);
  reader->read_rows(row_start, row_end, rows);
}

/*
 * When sorting, we organize the data as a pair of
 * {sort_keys, string of serialized values}.
 * But when writing we need to convert it back to a vector<flexible_type>
 *
 * This function destructively modifies sort_row to turn it into a
 * vector<flexible_type>.
 */
static void sort_row_to_output_row(std::pair<flex_list, std::string>& sort_row,
                                   std::vector<flexible_type>& output_row,
                                   size_t num_columns) {
  output_row.resize(num_columns);
  size_t i = sort_row.first.size();
  // first the key columns
  std::move(sort_row.first.begin(), sort_row.first.end(), output_row.begin());
  // then the value columns
  iarchive iarc(sort_row.second.c_str(), sort_row.second.length());
  for(; i < num_columns; ++i) {
    iarc >> output_row[i];
  }
}

/**
 * Moves permuted_row[permute_order[i]] to output_row[i]
 * permuted_row and output_row must not be the same object.
 */
static void permute_row(std::vector<flexible_type>& permuted_row,
                        std::vector<flexible_type>& output_row,
                        const std::vector<size_t>& permute_order) {
  // we can do this with an inplace permutation. But that is somewhat annoying
  // to implement
  output_row.resize(permuted_row.size());
  for (size_t i = 0;i < output_row.size(); ++i) {
    output_row[i] = std::move(permuted_row[permute_order[i]]);
  }
}

static void write_one_chunk(
  std::unique_ptr<sarray_reader<std::pair<flex_list, std::string>>>& reader,
  const std::vector<size_t>& permute_order,
  size_t segment_id,
  size_t num_columns,
  sframe_output_iterator& output_iterator) {

  flex_list permuted_row(num_columns);
  flex_list output_row(num_columns);
  for(auto iter = reader->begin(segment_id); iter != reader->end(segment_id); ++iter) {
    auto row = std::move(*iter);
    sort_row_to_output_row(row, permuted_row, num_columns);
    permute_row(permuted_row, output_row, permute_order);
    *output_iterator = output_row;
    output_iterator++;
  }
}

static void write_one_chunk(
    std::vector<std::pair<flex_list, std::string>>& rows,
    const std::vector<size_t>& permute_order,
    sframe_output_iterator& output_iterator,
    size_t num_columns) {
  std::vector<flexible_type> permuted_row(num_columns);
  std::vector<flexible_type> output_row(num_columns);
  for(auto& row : rows) {
    sort_row_to_output_row(row, permuted_row, num_columns);
    permute_row(permuted_row, output_row, permute_order);
    *output_iterator = output_row;
    output_iterator++;
  }
}


/**
 * Multithreaded sort implementation.
 * High-level algorithm understanding: Each segment's sort key range is
 * disjoint from all other segments, but the keys are unsorted with the
 * segment.  This allows us to sort multiple segments at once and output
 * the sorted ranges in segment order.
 *
 * Implementation: This is designed so that in the ideal case, N segments
 * will be able to fit in the buffer we're given. Threads wait on the
 * condition that there is enough memory for the segment they are assigned.
 * If a thread's segment is too large for our buffer, that thread must wait
 * until no other threads are running, and then it will take up the whole
 * buffer to sort...hopefully not allocating too much memory :/.
 */
std::shared_ptr<sframe> sort_and_merge(
    const std::shared_ptr<sarray<std::pair<flex_list, std::string>>>& partition_array,
    const std::vector<bool>& partition_sorted,
    const std::vector<size_t>& partition_sizes,
    const std::vector<bool>& sort_orders,
    const std::vector<size_t>& permute_order,
    const std::vector<std::string>& column_names,
    const std::vector<flex_type_enum>& column_types) {

  size_t num_segments = partition_array->num_segments();
  auto reader = partition_array->get_reader();
  atomic<size_t> next_segment_to_sort = 0;
  turi::mutex mem_used_mutex;
  turi::conditional mem_threshold_cv;
  size_t mem_used = 0;
  size_t num_threads = thread::cpu_count();

  // Prepare the output sframe
  sframe out_sframe;
  out_sframe.open_for_write(column_names, column_types, "", num_segments);
  size_t num_columns = column_names.size();
  less_than_full_function comparator(sort_orders);

  parallel_for(0, num_threads,
   [&](size_t thread_id) {
    // Each thread keep running until no more segment to sort
    std::vector<std::pair<flex_list, std::string>> rows;
    size_t segment_id = next_segment_to_sort++;
    while(segment_id < num_segments) {
      auto outiterator = out_sframe.get_output_iterator(segment_id);
      if (partition_sorted[segment_id]) {
        logstream(LOG_INFO) << "segment " << segment_id << " is already sorted, skip sorting " << std::endl;
        write_one_chunk(reader, permute_order, segment_id, num_columns, outiterator);
      } else {
        mem_used_mutex.lock();
        while((mem_used+partition_sizes[segment_id]) > sframe_config::SFRAME_SORT_BUFFER_SIZE) {
          if(((partition_sizes[segment_id] > sframe_config::SFRAME_SORT_BUFFER_SIZE) && (mem_used == 0)) ||
            (partition_sizes[segment_id] == 0)) {
            break;
          }
          mem_threshold_cv.wait(mem_used_mutex);
        }
        //logstream(LOG_INFO) << "sorting segment " << segment_id << " in thread " << thread_id << std::endl;
        mem_used += partition_sizes[segment_id];
        mem_used_mutex.unlock();
        read_one_chunk(reader, segment_id, num_columns, rows);

        // sort one chunk
        std::sort(rows.begin(), rows.end(), comparator);

        write_one_chunk(rows, permute_order ,outiterator, num_columns);
        out_sframe.flush_write_to_segment(segment_id);
        logstream(LOG_INFO) << "Finished sorting segment " << segment_id << std::endl;

        mem_used_mutex.lock();
        mem_used -= partition_sizes[segment_id];
        mem_threshold_cv.signal();
        mem_used_mutex.unlock();
      }
      segment_id = next_segment_to_sort++;
    }
  });
  out_sframe.close();
  return std::make_shared<sframe>(out_sframe);
}


} // end query_eval
} // end turicreate
