/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FAST_INTEGER_POWER_H_
#define TURI_FAST_INTEGER_POWER_H_

#include <iostream>
#include <array>
#include <algorithm>
#include <core/util/bitops.hpp>
#include <core/util/code_optimization.hpp>

namespace turi {

/**
 * \ingroup util
 * A class that gives highly optimized version of the power function
 *  for when a^b needs to be computed for many different values of b.
 *  b must be a non-negative integer (type size_t).
 *
 *  Usage:
 *
 *     fast_integer_power fip(a); // Slower than std::pow by a factor
                                  // of 10 or so.
 *
 *     fip.pow(b);   // Very fast, returns std::pow(a, b);
 *
 *  Internals:
 *
 *   Let the size_t value x be laid out in 4 bit blocks on disk.
 *
 *   | b0 b1 b2 b3 | b4 b5 b6 b7 | ... |
 *
 *   +-------------+-------------+ ...
 *     block 1        block 2
 *
 *   First note that a^b can be computed for by looking at all the bits of
 *   b that are 1, e.g. if b0, b4, and b5, are 1 (so b = 2^0 + 2^4 + 2^5),
 *   then a^b = a^(2^0) * a^(2^4) * a^(2^5).
 *
 *   Thus caching the values for each of these powers of 2 gives us the
 *   final value of a.
 *
 *   The method used internally employs this technique, but treats each
 *   block as a unit.  Thus we actually compute
 *
 *   a^b = a^(lookup1[block1 bits]) * a^(lookup2[block2 bits]) * ...
 *
 *   with each lookup having 8 bits and thus 256 possible values.
 */
class fast_integer_power {
 public:

  /** Constructs lookup tables to return std::pow(a, b).
   */
  fast_integer_power(double a = 1.0) {
    set_base(a);
  }

  /** Sets the base of the power function.
   */
  void set_base(double a) {
    _setup_block_lookups(a);
  }

  /** Returns a^b, where a is given in the constructor.
   */
  inline double pow(size_t b) const GL_HOT_INLINE_FLATTEN;

 private:

  // Must be greater than 2
  static constexpr size_t bits_per_block = 8;
  static constexpr size_t bit_selector = (1 << bits_per_block) - 1;
  static constexpr size_t first_level_size = (bitsizeof(size_t) + bits_per_block - 1) / bits_per_block;

  std::array<std::array<double, (1 << bits_per_block)>, first_level_size> block_lookups;

  /** Set up all lookups.
   */
  inline void _setup_block_lookups(double v);

};

////////////////////////////////////////////////////////////////////////////////
// All the functions definitions of the stuff above


/** Calculate out a^n.
 */
inline double fast_integer_power::pow(size_t n) const {
  double v = 1.0;
  for (size_t i = 0; i < bitsizeof(size_t) / bits_per_block; ++i) {
    v *= block_lookups[i][n & bit_selector];
    n >>= bits_per_block;
    if (n == 0) break;
  }

  return v;
}


/** Set up all lookups.
 */
inline void fast_integer_power::_setup_block_lookups(double v) {

  std::array<double, bitsizeof(size_t)> power_lookup;

  // First set up a lookup table by all powers of 2 in the mix.
  power_lookup[0] = v;
  for(size_t i = 1; i < bitsizeof(v); ++i)
    power_lookup[i] = power_lookup[i - 1] * power_lookup[i - 1];

  // Now build in the block-style lookup tables.
  for(size_t main_level = 0; main_level < first_level_size; ++main_level) {
    size_t offset = main_level * bits_per_block;

    for(size_t second_level = 0; second_level < (1 << bits_per_block); ++second_level) {

      double vp = 1.0;

      for(size_t bit_idx = 0;
          bit_idx < bits_per_block && offset + bit_idx < bitsizeof(size_t);
          ++bit_idx) {

        if(second_level & (1 << bit_idx) )
          vp *= power_lookup[offset + bit_idx];
      }

      block_lookups[main_level][second_level] = vp;
    }
  }
}

}

#endif /* TURI_FAST_INTEGER_POWER_H_ */
