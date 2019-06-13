/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/iterators/ml_data_block_iterator.hpp>

using namespace turi::v2::ml_data_internal;

namespace turi { namespace v2 {

void ml_data_block_iterator::internal_setup(const std::map<std::string, flexible_type>& options) {

  ASSERT_MSG((rm.metadata_vect.size() > 1
              && rm.metadata_vect[0]->mode == ml_column_mode::CATEGORICAL),
             "Block iterator can only be used if the first column is categorical.");
}


/// Loads the starting block, resetting all the values.
void ml_data_block_iterator::reset() {

  // If there's nothing here, don't bother doing any work
  if(iter_row_index_start == iter_row_index_end) {
    // Set this so done() says we are done
    current_row_is_start_of_new_block = true;
    current_row_index = iter_row_index_start;
    DASSERT_TRUE(done());
    return;
  }

  // Advance as needed to find the next block boundary.
  bool advance_needed;

  if(global_row_start == iter_row_index_start) {
    current_row_index = global_row_start;
    advance_needed = false;
  } else {
    DASSERT_GT(iter_row_index_start, global_row_start);

    current_row_index = iter_row_index_start - 1;
    advance_needed = true;
  }

  setup_block_containing_current_row_index();

  // We need to advance past the end of the current block and to the
  // start of the next one.
  if(advance_needed) {
    current_row_is_start_of_new_block = false;
    while(!done()) {
      ++(*this);
      if(is_start_of_new_block())
        break;
    }

  } else {
    // If we are at the start, then it's also the start of a new block.
    current_row_is_start_of_new_block = true;
  }
}

}}
