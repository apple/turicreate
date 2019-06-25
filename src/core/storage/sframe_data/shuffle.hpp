/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_SHUFFLE_HPP
#define TURI_SFRAME_SHUFFLE_HPP

#include <vector>
#include <core/storage/sframe_data/sframe.hpp>

namespace turi {

/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */


/**
 * Shuffle the rows in one sframe into a collection of n sframes.
 * Each output SFrame contains one segment.
 *
 * \code
 * std::vector<sframe> ret(n);
 * for (auto& sf : ret) {
 *   INIT_WITH_NAMES_COLUMNS_AND_ONE_SEG(sframe_in.column_names(), sframe_in.column_types());
 * }
 * for (auto& row : sframe_in) {
 *   size_t idx = hash_fn(row) % n;
 *   add_row_to_sframe(ret[idx], row); // the order of addition is not guaranteed.
 * }
 * \endcode
 *
 * The result sframes have the same column names and types (including
 * empty sframes). A result sframe can have 0 rows if non of the
 * rows in the input sframe is hashed to it. (If n is greater than
 * the size of input sframe, there will be at (n - sframe_in.size())
 * empty sframes in the return vector.
 *
 * \param n the number of output sframe.
 * \param hash_fn the hash function for each row in the input sframe.
 *
 * \return A vector of n sframes.
 */
std::vector<sframe> shuffle(
     sframe sframe_in,
     size_t n,
     std::function<size_t(const std::vector<flexible_type>&)> hash_fn,
     std::function<void(const std::vector<flexible_type>&, size_t)> emit_call_back
      = std::function<void(const std::vector<flexible_type>&, size_t)>());

/// \}
//
} // turi

#endif
