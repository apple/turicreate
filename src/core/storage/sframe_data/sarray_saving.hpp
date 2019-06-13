/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SARRAY_SAVING_HPP
#define TURI_SARRAY_SAVING_HPP
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/sarray_v2_block_manager.hpp>
#include <core/storage/sframe_data/sframe_saving_impl.hpp>
namespace turi {

template <typename T>
void sarray_save_blockwise(const sarray<T>& cur_column,
                           std::string index_file) {
  // initialize reader and writer
  auto& block_manager = v2_block_impl::block_manager::get_instance();
  v2_block_impl::block_writer writer;

  // call it indexfile.0000
  std::string base_name;
  size_t last_dot = index_file.find_last_of(".");
  if (last_dot != std::string::npos) {
    base_name = index_file.substr(0, last_dot);
  } else {
    base_name = index_file;
  }
  auto index = base_name + ".sidx";
  auto segment_file = base_name + ".0000";
  // we are going to emit only 1 segment. We should be rather IO bound anyway
  // and 1 column
  writer.init(index, 1, 1);
  writer.open_segment(0, segment_file);


  sframe_saving_impl::column_blocks col;
  // this is going to be a max heap with each entry referencing a column.
  try {
    col.column_index = cur_column.get_index_info();
    if (col.column_index.segment_files.size() > 0) {
      col.segment_address =
          block_manager.open_column(col.column_index.segment_files[0]);
      // the block address is basically a tuple beginning with
      // the column address
      col.num_blocks_in_current_segment =
          block_manager.num_blocks_in_column(col.segment_address);
      col.next_row = 0;
      col.column_number = 0;
      col.num_segments = col.column_index.segment_files.size();
      if (col.current_block_number >= col.num_blocks_in_current_segment) {
        advance_column_blocks_to_next_block(block_manager, col);
      }
    }


    writer.get_index_info().columns[0].metadata = col.column_index.metadata;

    while(!col.eof) {
      // read a block
      v2_block_impl::block_info* infoptr = nullptr;
      v2_block_impl::block_info info;
      v2_block_impl::block_address block_address
                            {std::get<0>(col.segment_address),
                              std::get<1>(col.segment_address),
                              col.current_block_number};
      auto data = block_manager.read_block(block_address , &infoptr);
      info = *infoptr;
      // write to segment 0. We have only 1 segment
      writer.write_block(0, col.column_number, data->data(), info);
      // increment the block number
      advance_column_blocks_to_next_block(block_manager, col);
      // if there are still blocks. push it back
    }

    // close writers.
    writer.close_segment(0);
    writer.write_index_file();
  } catch (...) {
    // cleanup. close any open columns
    try {
      block_manager.close_column(col.segment_address);
    } catch (...) { }
    throw;
  }
}
}
#endif
