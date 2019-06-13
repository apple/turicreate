/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <sstream>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sarray_v2_block_manager.hpp>
#include <core/storage/sframe_data/sarray_v2_block_types.hpp>
namespace turi {
namespace sframe_saving_impl {
void advance_column_blocks_to_next_block(
    v2_block_impl::block_manager& block_manager,
    column_blocks& block) {

  ++block.current_block_number;
  if (block.current_block_number >= block.num_blocks_in_current_segment) {
    // we need to advance to the next segment
    // close the current segment address
    block_manager.close_column(block.segment_address);
    while(1) {
      // advance to the next segment if there is one
      block.current_block_number = 0;
      ++block.current_segment_number;
      if (block.current_segment_number < block.num_segments) {
        // open then ext segment
        auto current_segment_file =
            block.column_index.segment_files[block.current_segment_number];
        block.segment_address = block_manager.open_column(current_segment_file);
        block.num_blocks_in_current_segment =
            block_manager.num_blocks_in_column(block.segment_address);
        // segment is empty. keep going...
        if (block.num_blocks_in_current_segment == 0) {
          block_manager.close_column(block.segment_address);
          continue;
        }
        break;
      } else {
        block.eof = true;
        break;
      }
    }
  }
}
} // sframe_saving_impl
} // namespace turi
