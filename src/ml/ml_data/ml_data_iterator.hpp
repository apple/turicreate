/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_ITERATOR_H_
#define TURI_DML_DATA_ITERATOR_H_

#include <core/logging/assertions.hpp>
#include <ml/ml_data/data_storage/ml_data_row_translation.hpp>
#include <ml/ml_data/data_storage/ml_data_block_manager.hpp>
#include <ml/ml_data/ml_data.hpp>
#include <ml/ml_data/row_reference.hpp>
#include <core/util/code_optimization.hpp>

// SArray and Flex type
#include <core/storage/sframe_data/sarray.hpp>

#include <Eigen/SparseCore>
#include <Eigen/Core>

#include <array>

namespace turi {

class ml_data;

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

/**
 * \ingroup mldata
 * Just a simple iterator on the ml_data class.  It's just a
 * convenience structure that keeps track of everything relevant for
 * the toolkits.
 */
class ml_data_iterator {
 private:

  // To be initialized only from the get_iterator() method of ml_data.
  friend class ml_data;

  // Internal -- called from ml_data;
  void setup(const ml_data& _data,
             const ml_data_internal::row_metadata& rm,
             size_t thread_idx, size_t num_threads);

 public:

  ml_data_iterator(){}
  ml_data_iterator(const ml_data_iterator&) = delete;
  ml_data_iterator(ml_data_iterator&&) = default;

  ml_data_iterator& operator=(const ml_data_iterator&) = delete;
  ml_data_iterator& operator=(ml_data_iterator&&) = default;

  ///  Resets the iterator to the start of the sframes in ml_data.
  void reset();

  /// Returns true if the iteration is done, false otherwise.
  inline bool done() const { return current_row_index == iter_row_index_end; }

  /// Returns the current index of the sframe row, respecting all
  /// slicing operations on the original ml_data.
  inline size_t row_index() const { return current_row_index - global_row_start; }

  ////////////////////////////////////////////////////////////////////////////////

  /**  Return a row reference.  The row reference can be used to fill
   *   the observation vectors.
   */
  ml_data_row_reference operator*() const GL_HOT_INLINE_FLATTEN {
    return row;
  }

  /**  Dereference the iterator.
   */
  ml_data_row_reference const* operator->() const GL_HOT_INLINE_FLATTEN {
    return &row;
  }


  ////////////////////////////////////////////////////////////////////////////////

  /// Advance the iterator to the next observation.
  const ml_data_iterator& operator++() GL_HOT_INLINE_FLATTEN {
    this->advance_row();
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////

  /** Return the data this iterator is working with.
   */
  inline const ml_data& ml_data_source() const {
    return data;
  }

  /** Return the raw value of the internal row storage.  Used by some
   *  of the internal ml_data processing routines.
   */
  inline ml_data_internal::entry_value _raw_row_entry(size_t raw_index) const GL_HOT_INLINE_FLATTEN {

    if(!rm.data_size_is_constant)
      ++raw_index;

    return *(current_data_iter() + raw_index);
  }

  ////////////////////////////////////////////////////////////////////////////////

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

 private:

  // Internally, ml_data is just a bunch of shared pointers, so it's
  // not expensive to store a copy.
  ml_data data;

  ml_data_internal::row_metadata rm;

  size_t iter_row_index_start = -1;   /**< Starting row index for this iterator. */
  size_t iter_row_index_end = -1;     /**< Ending row index for this iterator. */
  size_t current_row_index = -1;      /**< Current row index for this iterator. */
  size_t current_block_index = -1;    /**< Index of the currently loaded block. */

  /** A reference to the current row that we're pointing to.  Holds
   * the data_block and current_in_block_index
   */
  ml_data_row_reference row;

  /** The absolute values of the global row starting locations.
   */
  size_t global_row_start, global_row_end;

 private:

  /** Return a pointer to the current location in the data.
   */
  inline ml_data_internal::entry_value_iterator current_data_iter() const GL_HOT_INLINE_FLATTEN {

    DASSERT_FALSE(done());
    DASSERT_LT(row.current_in_block_index, row.data_block->translated_rows.entry_data.size());

    return &(row.data_block->translated_rows.entry_data[row.current_in_block_index]);
  }

  /** Return a pointer to the current location in the data.
   */
  inline size_t current_block_row_index() const GL_HOT_INLINE_FLATTEN {

    size_t index = current_row_index - (current_block_index * data.row_block_size);

    DASSERT_FALSE(done());
    DASSERT_LT(index, data.row_block_size);

    return index;
  }


  /** Advance to the next row.
   */
  inline void advance_row() GL_HOT_INLINE_FLATTEN {

    if(row.has_translated_columns || rm.has_target)
      row.current_in_block_index += get_row_data_size(rm, current_data_iter());

    ++current_row_index;

    DASSERT_GE(current_row_index, current_block_index * data.row_block_size);

    row.current_in_block_row_index = current_row_index - current_block_index * data.row_block_size;

    if(row.current_in_block_row_index == data.row_block_size && !done())
      load_next_block();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Internal reader functions

  /// Loads the block containing the row index row_index
  void setup_block_containing_current_row_index() GL_HOT_NOINLINE;

  /// Loads the next block, resetting all the values so iteration will
  /// be supported over the next row.
  void load_next_block() GL_HOT_NOINLINE;

};

}

#endif /* TURI_DML_DATA_ITERATOR_H_ */
