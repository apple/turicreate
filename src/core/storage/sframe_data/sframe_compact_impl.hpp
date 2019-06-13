/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_COMPACT_IMPL_HPP
#define TURI_SFRAME_COMPACT_IMPL_HPP
#include <vector>
#include <memory>
#include <core/storage/sframe_data/sarray.hpp>
namespace turi {
namespace {
/**
 * Returns the number of blocks in a segment file
 */
inline size_t get_num_blocks_in_segment_file(const std::string& s) {
  auto& manager = v2_block_impl::block_manager::get_instance();
  auto columnaddr =  manager.open_column(s);
  return manager.num_blocks_in_column(columnaddr);
}

template <typename T>
inline std::shared_ptr<sarray<T>>
compact_rows(sarray<T>& arr, size_t row_start, size_t row_end) {
  // returned sarray
  auto ret = std::make_shared<sarray<T>>();
  ret->open_for_write(1);
  auto output = ret->get_output_iterator(0);

    // read input array
  auto reader = arr.get_reader();
  sframe_rows rows;
  while(row_start < row_end) {
    size_t read_end = std::min(row_start + DEFAULT_SARRAY_READER_BUFFER_SIZE, row_end);
    bool read_ok = reader->read_rows(row_start, read_end, rows);
    ASSERT_TRUE(read_ok);

    // write output array
    (*output) = rows;
    row_start = read_end;
  }
  ret->close();
  return ret;
}

} // anonymous namespace

template <typename T>
bool sarray_fast_compact(sarray<T>& column) {

  auto index = column.get_index_info();

  // this is the resultant index
  auto updated_index = index;
  updated_index.segment_sizes.clear();
  updated_index.segment_files.clear();

  size_t row_counter = 0;
  bool compaction_performed = false;

  // we keep a buffer of the new sarrays constructed after compaction until we
  // actually piece them together. To avoid them going out of scope and
  // and clearing the data.
  std::vector<std::shared_ptr<sarray<T>>> new_sarrays;

  for (size_t i = 0;i < index.segment_files.size(); ++i) {
    size_t nblocks = get_num_blocks_in_segment_file(index.segment_files[i]);
    if (nblocks < FAST_COMPACT_BLOCKS_IN_SMALL_SEGMENT) {
      // find a run of block size 1
      size_t runlength_in_segments = 1;
      size_t runlength_in_rows = index.segment_sizes[i];
      for (size_t j = i + 1; j < index.segment_files.size(); ++j) {
        size_t nblocks = get_num_blocks_in_segment_file(index.segment_files[j]);
        if (nblocks <= 1) {
          runlength_in_rows += index.segment_sizes[j];
          ++runlength_in_segments;
        } else {
          break;
        }
      }
      // we only compact if there is more than one segment of block size 1
      if (runlength_in_segments > 1) {
        logstream(LOG_INFO) << "Compacting range of "
                            << runlength_in_segments << " blocks, "
                            << runlength_in_rows << " rows" << std::endl;

        // we compact the segment range
        auto new_sarray = compact_rows(column, row_counter, row_counter + runlength_in_rows);
        // put it into the updated index
        auto new_sarray_index = new_sarray->get_index_info();
        ASSERT_EQ(new_sarray_index.segment_files.size(), 1);
        ASSERT_EQ(new_sarray_index.segment_sizes[0], runlength_in_rows);
        row_counter += runlength_in_rows;
        updated_index.segment_sizes.push_back(new_sarray_index.segment_sizes[0]);
        updated_index.segment_files.push_back(new_sarray_index.segment_files[0]);

        //remember the new sarray so that it doesn't go out of scope
        //until we actually construct the result.
        compaction_performed = true;
        new_sarrays.push_back(new_sarray);
        // increment the index by the right amount so that we end up
        // on the segment after the run
        i += runlength_in_segments - 1;
        continue;
      }
    }
    row_counter += index.segment_sizes[i];
    updated_index.segment_sizes.push_back(index.segment_sizes[i]);
    updated_index.segment_files.push_back(index.segment_files[i]);
  }

  if (compaction_performed) {
    sarray<T> final_array;
    updated_index.nsegments = updated_index.segment_files.size();
    final_array.open_for_read(updated_index);
    ASSERT_EQ(final_array.size(), column.size());
    column = final_array;
  }
  return compaction_performed;
}


template <typename T>
void sarray_compact(sarray<T>& column, size_t segment_threshold) {
  sarray_fast_compact(column);
  if (column.get_index_info().segment_files.size() > segment_threshold) {
    logstream(LOG_INFO)
        << "Slow compaction triggered because fast compact did not achieve target"
        << std::endl;
    column = *(column.clone(std::min(segment_threshold, thread::cpu_count())));
  }
}

} // turi
#endif
