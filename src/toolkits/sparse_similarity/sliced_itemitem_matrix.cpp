/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/sparse_similarity/sliced_itemitem_matrix.hpp>
#include <vector>
#include <cmath>


namespace turi { namespace sparse_sim {

/** The height of a given slice that excludes the items below the
 *  diaganol.   Assumed to be at least as wide as it is high.
 */
size_t get_upper_triangular_slice_height(size_t _w, size_t _s) {
  double w = _w;
  double s = _s;

  if( (w * (w + 1)) / 2 <= s) {
    return w;
  }

  // The area of the slice is s = h * (h + 1) / 2 + (w-h) * h; this is
  // solving for h from s and w.

  return std::max<size_t>
      (1, size_t(std::floor((w + 0.5) - std::sqrt( std::pow(w + 0.5, 2) - 2 * s))));
}

/** Calculates the number of passes required to go through a symmetric
 *  or triangular matrix with a fixed amount of memory.  To calculate
 *  the number of slices required, as well as the slice boundaries, we
 *  iteratively:
 *
 *   1. Process the first b rows of num_items by num_items matrix.
 *   2. Drop the left most b rows of the matrix.
 *   3. Repeat on remaining (num_items - b) by (num_items - b) matrix.
 *
 *  This allows us to effeciently slice up a large triangular matrix.

 *  For example, if it is a 16 by 16 matrix, with
 *  target_item_count_per_pass = 16, then there would be 11 slices --
 *  the first 8 rows would have to be processed individually, as only
 *  one row of the upper triangular part (including diaganol) would
 *  fit in that 16 elements at a time.  The next two slices would each
 *  consist of 2 rows, as each row would have less than 8 elements in
 *  at a time, and the final 4 x 4 slice can be done in one go.
 */
std::vector<size_t> calculate_upper_triangular_slice_structure(
    size_t num_items, size_t target_item_count_per_pass, size_t max_num_slices) {

  DASSERT_GE(num_items, 1);

  std::vector<size_t> slice_boundaries;
  size_t base_index = 0;
  size_t n = num_items;
  size_t n_slices = 0;

  while(true) {
    size_t b = get_upper_triangular_slice_height(n, target_item_count_per_pass);

    slice_boundaries.push_back(base_index);
    ++n_slices;

    if(n_slices > max_num_slices) {
      // This is our error code -- can't do it in this many slices!
      slice_boundaries.clear();
      break;
    }

    if(n <= b) {
      slice_boundaries.push_back(num_items);
      break;
    }

    base_index += b;
    n -= b;
  }

  return slice_boundaries;
}

}}
