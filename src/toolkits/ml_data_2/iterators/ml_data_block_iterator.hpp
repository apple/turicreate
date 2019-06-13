/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_BLOCK_ITERATOR_H_
#define TURI_ML2_DATA_BLOCK_ITERATOR_H_

#include <toolkits/ml_data_2/iterators/ml_data_iterator.hpp>


namespace turi { namespace v2 {

/**  This iterator acts similarly to the regular ml_data_iterator
 *   class; however, it also enables the user to implement simple
 *   iteration over blocks of rows.  Here, a block is defined by a
 *   collection of rows in which the first value is common.
 *
 *   The ml_data_block_iterator does this by providing two additional
 *   functionalities beyond ml_data_iterator:
 *
 *   1.  is_start_of_new_block() returns true only if the first value
 *   in the current row differs from the first value in the previous
 *   row.  (It is also true at the starting bound of iteration).  Thus
 *   the user knows when to switch to a new block.
 *
 *   2.  If the iteration range is broken up by threads,
 *   i.e. num_threads > 1, then the effective bounds of the partitions
 *   of each individual iterator will always be on the boundaries
 *   between blocks.  Thus parallel iteration will never split a block
 *   between two threads.
 *
 *   In all other respects, this iterator behaves just like
 *   ml_data_iterator.
 */
class ml_data_block_iterator final : public ml_data_iterator_base {
public:

  void internal_setup(const std::map<std::string, flexible_type>& options);

  /** This function returns true if the current observation is the
   *  start of a new block.
   */
  bool is_start_of_new_block() const {
    return current_row_is_start_of_new_block;
  }

  /** Advance the iterator to the next row.
   */
  inline const ml_data_block_iterator& operator++() GL_HOT_INLINE_FLATTEN {
    DASSERT_LT(current_row_index, global_row_end);

    size_t old_entry = _raw_row_entry(0).index_value;

    // If this flag is true, then the iterator falsely thinks we're
    // done and doesn't load a new block
    current_row_is_start_of_new_block = false;

    DASSERT_LT(current_row_index, global_row_end);

    advance_row();

    DASSERT_LE(current_row_index, global_row_end);

    if(!done()) {
      size_t current_entry = _raw_row_entry(0).index_value;

      current_row_is_start_of_new_block = (current_entry != old_entry);

    } else {
      current_row_is_start_of_new_block = true;
    }

    return *this;
  }

  /**
   *   Resets the iterator to the start of the sframes in ml_data.
   */
  void reset();

  /**  Returns true if we are done with the iteration range of the
   *  current iterator and false otherwise.
   */
  inline bool done() const GL_HOT_INLINE_FLATTEN {

    DASSERT_LE(current_row_index, global_row_end);

    // We aren't done until we hit the end of a block
    return (current_row_index == global_row_end ||
            (current_row_index >= iter_row_index_end
             && current_row_is_start_of_new_block));
  }

private:

  // Set by operator++() or true at the very start of the larger
  // block.
  bool current_row_is_start_of_new_block;
};

}}

#endif
