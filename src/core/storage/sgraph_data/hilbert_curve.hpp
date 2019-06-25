/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_HILBERT_CURVE_HPP
#define TURI_SGRAPH_HILBERT_CURVE_HPP
#include <utility>
#include <cstdint>
#include <cstddef>
#include <core/logging/assertions.hpp>
#include <core/util/bitops.hpp>
namespace turi {


/**
 * \internal
 * \ingroup sgraph_physical
 * \addtogroup sgraph_compute SGraph Compute
 * \{
 */

/**
 * For an n*n square and a hilbert index (s) ranging from 0 to n*n-1, this
 * function returns the coordinate of the s^th position along the hilbert
 * curve. n must be at least 2 and must be a power of 2.
 *
 * Algorithm from Figure 14-8 in Hacker's Delight.
 */
inline std::pair<size_t, size_t> hilbert_index_to_coordinate(size_t s, size_t n) {
  if (s == 0 && n == 1) return {0, 0};

  ASSERT_GE(n, 2);
  ASSERT_TRUE(is_power_of_2(n));
  n = __builtin_ctz(n); // convert to the "Order" of the curve. i.e. log(n)
  size_t comp, swap, cs, t, sr;

  s = s | (0x55555555 << 2*n); // Pad s on left with 01
  sr = (s >> 1) & 0x55555555;  // (no change) groups.
  cs = ((s & 0x55555555) + sr) // Compute complement &
       ^ 0x55555555;           // swap info in two-bit
                               // groups.
  // Parallel prefix xor op to propagate both complement
  // and swap info together from left to right (there is
  // no step "cs ^= cs >> 1", so in effect it computes
  // two independent parallel prefix operations on two
  // interleaved sets of sixteen bits).

  cs = cs ^ (cs >> 2);
  cs = cs ^ (cs >> 4);
  cs = cs ^ (cs >> 8);
  cs = cs ^ (cs >> 16);
  swap = cs & 0x55555555;      // Separate the swap and
  comp = (cs >> 1) & 0x55555555;  // complement bits.

  t = (s & swap) ^ comp;       // Calculate x and y in
  s = s ^ sr ^ t ^ (t << 1);   // the odd & even bit
                               // positions, resp.
  s = s & ((1 << 2*n) - 1);    // Clear out any junk
                               // on the left (unpad).

  // Now "unshuffle" to separate the x and y bits.

  t = (s ^ (s >> 1)) & 0x22222222; s = s ^ t ^ (t << 1);
  t = (s ^ (s >> 2)) & 0x0C0C0C0C; s = s ^ t ^ (t << 2);
  t = (s ^ (s >> 4)) & 0x00F000F0; s = s ^ t ^ (t << 4);
  t = (s ^ (s >> 8)) & 0x0000FF00; s = s ^ t ^ (t << 8);
  return {s >> 16, s & 0xFFFF};
}


/**
 * For an n*n square and a coordinate within the square. Returns the
 * hilbert index  which is the position of the coordinate along the hilbert curve.
 * n must be at least 2 and must be a power of 2.
 *
 * Algorithm from Figure 14-9 in Hacker's Delight.
 */
inline size_t coordinate_to_hilbert_index(std::pair<size_t, size_t> coord, size_t n) {
  if ((n == 1) && (coord.first == 0) && (coord.second = 0)) return 0;

  ASSERT_GE(n, 2);
  ASSERT_TRUE(is_power_of_2(n));
  ASSERT_LT(coord.first, n);
  ASSERT_LT(coord.second, n);

  n = __builtin_ctz(n); // convert to the "Order" of the curve. i.e. log(n)
  int i;
  size_t state, s, row;
  size_t x = coord.first;
  size_t y = coord.second;

  state = 0;                            // Initialize.
  s = 0;
  for (i = n - 1; i >= 0; i--) {
    row = 4*state | 2*((x >> i) & 1) | ((y >> i) & 1);
    s = (s << 2) | ((0x361E9CB4 >> 2*row) & 3);
    state = (0x8FE65831 >> 2*row) & 3;
  }
  return s;
}

/// \}
} // namespace turi
#endif // TURI_SGRAPH_ZORDER_HPP
