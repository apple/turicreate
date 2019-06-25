/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_ITERATOR_H_
#define TURI_ML2_DATA_ITERATOR_H_

#include <toolkits/ml_data_2/iterators/ml_data_iterator_base.hpp>

namespace turi { namespace v2 {

/** The implementation of the ml_data_iterator.  The main
 * functionality is in ml_data_iterator_base.
 */
class ml_data_iterator final : public ml_data_iterator_base {
 public:

  /// Advance the iterator to the next observation.
  const ml_data_iterator& operator++() GL_HOT_INLINE_FLATTEN {
    this->advance_row();
    return *this;
  }

  /** Seeks to the row given by row_index.
   *
   */
  void seek(size_t row_index) {
    size_t absolute_row_index = row_index + global_row_start;

    ASSERT_MSG(absolute_row_index <= global_row_end,
               "Requested row index out of bounds.");

    ASSERT_MSG((iter_row_index_start == global_row_start
                && iter_row_index_end == global_row_end),
               "Seek not supported with multithreaded iterators.");

    current_row_index = absolute_row_index;

    if(!done())
      setup_block_containing_current_row_index();
  }


};

}}

#endif /* TURI_ML2_DATA_ITERATOR_H_ */
