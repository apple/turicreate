/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SPARSE_SIM_SLICED_MATRIX_UTILITIES_H
#define TURI_SPARSE_SIM_SLICED_MATRIX_UTILITIES_H

#include <vector>
#include <core/logging/assertions.hpp>
#include <core/parallel/atomic.hpp>
#include <core/parallel/lambda_omp.hpp>

namespace turi { namespace sparse_sim {

/** The height of a given slice that excludes the items below the
 *  diaganol
 */
size_t get_upper_triangular_slice_height(size_t _w, size_t _s);

/** Calculates the number of passes required to go through a symmetric
 *  or triangular matrix with a fixed amount of memory.  We return the
 *  slice or slice boundaries needed in order to do a set of passes
 *  through the matrix such that everything fits in memory.
 */
std::vector<size_t> calculate_upper_triangular_slice_structure(
    size_t num_items, size_t target_item_count_per_pass, size_t max_num_slices);

/** A container to hold a slice of rows of an upper triangular dense
 *  matrix.  All accesses assume that row_idx < col_idx, and only data
 *  conforming to this is stored.  In addition, num_rows <= num_cols.
 *
 *  This class is a minimal wrapper around a vector in order to mimic
 *  the interface of the sparse item by item array.  This has the same
 *  interface as the symmetric_2d_array class.
 *
 */
template <typename T>
class dense_triangular_itemitem_container {
 private:
  static constexpr size_t _data_size(size_t n_rows, size_t n_cols) {
    return ((n_cols - 1) * n_rows) - (n_rows * (n_rows - 1)) / 2;
  }

 public:

  dense_triangular_itemitem_container(){}

  dense_triangular_itemitem_container(size_t _num_rows, size_t _num_cols) {
    resize(_num_rows, _num_cols);
  }

  typedef T value_type;

  /**  Clears all the data and the values, and resets the number of
   *  rows and columns to 0.
   *
   */
  void clear() {
    num_rows = 0;
    num_cols = 0;
    data.clear();
  }

  /** Reserve a fixed number of elements.
   */
  void reserve(size_t n_elements) {
    data.reserve(n_elements);
  }

  size_t rows() const { return num_rows; }
  size_t cols() const { return num_cols; }

  /**  Resize and clear the data.
   */
  void resize(size_t _num_rows, size_t _num_cols) {
    DASSERT_LE(_num_rows, _num_cols);
    num_cols = _num_cols;
    num_rows = _num_rows;
    size_t s = _data_size(num_rows, num_cols);
    data.assign(s, value_type());
    setup_row_index_map();
  }

  /** Indexible access.
   */
  value_type& operator()(size_t row_idx, size_t col_idx) {
    size_t index = data_index(row_idx, col_idx);
    return data[index];
  }

  /** Indexible access.  Const overload
   */
  const value_type& operator()(size_t i, size_t j) const {
    size_t index = data_index(i, j);
    return data[index];
  }

  /**  Apply a function to a particular element.  The apply_f has
   */
  template <typename ApplyFunction>
  GL_HOT_INLINE_FLATTEN
  void apply(size_t idx_1, size_t idx_2, ApplyFunction&& apply_f) {
    size_t index = data_index(idx_1, idx_2);
    apply_f(data[index]);
  }

  /**  Process all the elements currently in this container.
   */
  template <typename ProcessValueFunction>
  void apply_all(ProcessValueFunction&& process_interaction) {

    atomic<size_t> row_idx = 0;

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        while(true) {
          size_t idx_1 = (++row_idx) - 1;
          if(idx_1 >= num_rows) break;

          size_t idx_2 = idx_1 + 1;
          if(idx_2 >= num_cols) continue;

          value_type * __restrict__ d_ptr = data.data() + data_index(idx_1, idx_2);

          for(; idx_2 != num_cols; ++idx_2, ++d_ptr) {
            process_interaction(idx_1, idx_2, *d_ptr);
          }
        }
      });
  }

 private:
  size_t num_cols=0, num_rows=0;
  std::vector<value_type> data;
  std::vector<size_t> row_index_map;

  void setup_row_index_map() GL_HOT_INLINE_FLATTEN {
    row_index_map.resize(num_rows + 1);

    // The storage location is the number of elements before this,
    // minus the shift required to compensate for the col_idx being
    // larger than the row idx.
    for(size_t r = 0; r < num_rows; ++r) {
      row_index_map[r] = _data_size(r, num_cols) - r;
    }

    row_index_map[num_rows] = data.size();
  }

  /** Calculates the data index of a particular row and column.
   */
  size_t data_index(size_t row_idx, size_t col_idx) const GL_HOT_INLINE_FLATTEN {
    DASSERT_LT(row_idx, num_rows);
    DASSERT_LT(col_idx, num_cols);
    DASSERT_LT(row_idx, col_idx);

    size_t index = row_index_map[row_idx] + (col_idx - 1);
    DASSERT_LT(index, data.size());

    return index;
  }

};

}}

#endif /* SLICED_MATRIX_UTILITIES_H */
