/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/ml_data/ml_data_iterator.hpp>

using namespace turi::ml_data_internal;

namespace turi {

void ml_data_iterator::setup(const ml_data& _data,
                             const ml_data_internal::row_metadata& _rm,
                             size_t thread_idx,
                             size_t num_threads)
{
  // Set the internal data and options
  data                     = _data;
  rm                       = _rm;

  ////////////////////////////////////////////////////////////////////////////////
  // Set up the blocks of information
  global_row_start = data._row_start;
  global_row_end   = data._row_end;

  size_t n_idx = global_row_end - global_row_start;

  iter_row_index_start = global_row_start + (thread_idx * n_idx) / num_threads;
  iter_row_index_end   = global_row_start + ((thread_idx+1) * n_idx) / num_threads;

  row.has_untranslated_columns = data.metadata()->has_untranslated_columns();
  row.has_translated_columns = data.metadata()->has_translated_columns();

  // Reset to the beginning.
  reset();
}

////////////////////////////////////////////////////////////////////////////////

void ml_data_iterator::setup_block_containing_current_row_index() {

  // Check this silly corner case
  if(iter_row_index_start == iter_row_index_end) {
    row.data_block.reset();
    return;
  }

  if(current_row_index < current_block_index * data.row_block_size
     || current_row_index >= (current_block_index + 1) * data.row_block_size) {

    // Set the current block index
    current_block_index = current_row_index / data.row_block_size;

    row.data_block.reset();
    row.data_block = data.block_manager->get_block(current_block_index);
  }

  size_t desired_current_row = current_row_index;

  // This is actually where we are at -- the start of this block.
  current_row_index = current_block_index * data.row_block_size;

  // Set up to the actual current state, then skip rows til we got
  // what we want.
  row.current_in_block_index = 0;

  if(rm.data_size_is_constant) {

    row.current_in_block_index = rm.constant_data_size * (desired_current_row - current_row_index);
    current_row_index = desired_current_row;

  } else {

    while(current_row_index != desired_current_row) {

      row.current_in_block_index += get_row_data_size(rm, current_data_iter());
      ++current_row_index;

      DASSERT_FALSE(done());
      DASSERT_TRUE(row.current_in_block_index != row.data_block->translated_rows.entry_data.size());
    }
  }

  row.current_in_block_row_index = current_row_index - current_block_index * data.row_block_size;
}

/// Loads the next block, resetting all the values
void ml_data_iterator::reset() {
  current_row_index = iter_row_index_start;
  setup_block_containing_current_row_index();
}

/// skips the next row
void ml_data_iterator::load_next_block() {
  DASSERT_TRUE(current_row_index % data.row_block_size == 0);
  setup_block_containing_current_row_index();
}

}
