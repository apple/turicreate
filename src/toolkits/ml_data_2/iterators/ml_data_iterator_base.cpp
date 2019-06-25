/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/iterators/ml_data_iterator_base.hpp>

using namespace turi::v2::ml_data_internal;

namespace turi { namespace v2 {

void ml_data_iterator_base::setup(const ml_data& _data,
                                  const ml_data_internal::row_metadata& _rm,
                                  size_t thread_idx,
                                  size_t num_threads,
                                  const std::map<std::string, flexible_type>& options)
{
  // Set the internal data and options
  data.reset(new ml_data(_data));
  rm                       = _rm;

  use_reference_encoding   = options.at("use_reference_encoding");
  add_side_information     = data->has_side_features() && bool(options.at("add_side_information_if_present"));

  side_features            = (add_side_information

                             // Important to pull from here, not metadata, as they may be different
                             ? data->side_features
                             : std::shared_ptr<ml_data_side_features>());

  has_untranslated_columns = data->has_untranslated_columns();
  has_translated_columns   = data->has_translated_columns() | data->has_target();

  ////////////////////////////////////////////////////////////////////////////////
  // Set up the blocks of information
  global_row_start = data->_row_start;
  global_row_end   = data->_row_end;

  size_t n_idx = global_row_end - global_row_start;

  iter_row_index_start = global_row_start + (thread_idx * n_idx) / num_threads;
  iter_row_index_end   = global_row_start + ((thread_idx+1) * n_idx) / num_threads;

  max_row_size = data->max_row_size();
  row_block_size = data->row_block_size;

  num_dimensions = data->metadata()->num_dimensions();

  // Call the internal setup function
  internal_setup(options);

  // Reset to the beginning.
  reset();
}

////////////////////////////////////////////////////////////////////////////////

void ml_data_iterator_base::setup_block_containing_current_row_index() {

  // Check this silly corner case
  if(iter_row_index_start == iter_row_index_end) {
    data_block.reset();
    return;
  }

  if(current_row_index < current_block_index * row_block_size
     || current_row_index >= (current_block_index + 1) * row_block_size) {

    // Set the current block index
    current_block_index = current_row_index / row_block_size;

    data_block.reset();
    data_block = data->block_manager->get_block(current_block_index);
  }

  size_t desired_current_row = current_row_index;

  // This is actually where we are at -- the start of this block.
  current_row_index = current_block_index * data->row_block_size;

  // Set up to the actual current state, then skip rows til we got
  // what we want.
  current_in_block_index = 0;

  if(rm.data_size_is_constant) {

    current_in_block_index = rm.constant_data_size * (desired_current_row - current_row_index);
    current_row_index = desired_current_row;

  } else {

    while(current_row_index != desired_current_row) {

      current_in_block_index += get_row_data_size(rm, current_data_iter());
      ++current_row_index;

      DASSERT_FALSE(done());
      DASSERT_TRUE(current_in_block_index != data_block->translated_rows.entry_data.size());
    }
  }
}

/// Loads the next block, resetting all the values
void ml_data_iterator_base::reset() {
  current_row_index = iter_row_index_start;
  setup_block_containing_current_row_index();
}

/// skips the next row
void ml_data_iterator_base::load_next_block() {
  DASSERT_TRUE(current_row_index % row_block_size == 0);
  setup_block_containing_current_row_index();
}

}}
