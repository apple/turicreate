/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_BITOPS_H
#define TURI_BITOPS_H

#include <util/int128_types.hpp>
#include <cmath>
#include <type_traits>
#include <climits>
#include <util/code_optimization.hpp>
// #include <bitset>
// #include <iostream>

#define bitsizeof(t) (8*sizeof(t))

namespace turi {

#define _ENABLE_IF_UINT(T)                                              \
  typename std::enable_if<std::is_same<T, uint8_t>::value               \
                          || std::is_same<T, uint16_t>::value           \
                          || std::is_same<T, uint32_t>::value           \
                          || std::is_same<T, uint64_t>::value           \
                          || std::is_same<T, unsigned int>::value       \
                          || std::is_same<T, unsigned long>::value      \
                          || std::is_same<T, unsigned long long>::value \
                          || std::is_same<T, uint128_t>::value>::type* = 0

////////////////////////////////////////////////////////////
// For helping with some of the templating later on
template <typename T>
bool static constexpr __bitsize_gt(unsigned n) {
  return n < bitsizeof(T);
}

#define _ENABLE_IF_BITSIZE_GT(T, n)                             \
  typename std::enable_if<__bitsize_gt<T>(n)>::type* = 0

#define _ENABLE_IF_BITSIZE_LEQ(T, n)                            \
  typename std::enable_if<!__bitsize_gt<T>(n)>::type* = 0

////////////////////////////////////////////////////////////

/**
 * Tests if x is a power of 2 (i.e. if just one bit is on).
 *
 * \param x Unsigned integer to test.
 */
template <typename T>
static inline bool is_power_of_2(const T& x, _ENABLE_IF_UINT(T)) {
  return ( ! ((x) & ((x) - 1) ) );
}

/**
 * Returns true if a bit is on.   Other bits are ignored.
 *
 * \param x   Unsigned integer to test.
 * \param bit Index of the bit to test.
 */
template <typename T>
static inline bool bit_on(T x, unsigned int bit, _ENABLE_IF_UINT(T)) {
  return !!(x & (T(1) << bit));
}


/**
 * Returns true if a bit is off.   Other bits are ignored.
 *
 * \param x   Unsigned integer to test.
 * \param bit Index of the bit to test.
 */
template <typename T>
static inline bool bit_off(T x, unsigned int bit, _ENABLE_IF_UINT(T))
{
  return (x & (T(1) << bit) ) == 0;
}

/**
 * Sets a bit to be off.
 *
 * \param x   Reference to unsigned integer to change.
 * \param bit Index of the bit to set off.
 */
template <typename T>
static inline void set_bit_off(T& x, unsigned int bit, _ENABLE_IF_UINT(T))
{
  x &= (~((T(1) << bit)));
}

/**
 * Sets a bit to be on.
 *
 * \param x   Reference to unsigned integer to change.
 * \param bit Index of the bit to set on.
 */
template <typename T>
static inline void set_bit_on(T& x, unsigned int bit, _ENABLE_IF_UINT(T))
{
  x |= (T(1) << bit);
}

/**
 * Flips a bit.
 *
 * \param x   Reference to unsigned integer to change.
 * \param bit Index of the bit to flip.
 */
template <typename T>
static inline void flip_bit(T& x, unsigned int bit, _ENABLE_IF_UINT(T))
{
  x ^= (T(1) << bit);
}

/**
 * Returns a bitwise mask of the first n_bits customised to type T.
 * For example, bit_bask<uint16_t>(5) & (000101011101011101 b) returns
 * 11101 b.
 *
 * \tparam T     Type of the mask to be created.
 * \param n_bits Index of the bit to flip.
 */
template <typename T>
static inline T bit_mask(unsigned int n_bits, _ENABLE_IF_UINT(T)) {

  static constexpr unsigned int _n_mask = ~static_cast<unsigned int>(bitsizeof(T) - 1);

  return UNLIKELY(n_bits & _n_mask) ? T(-1) : (T(1) << n_bits) - 1;
}

/**
 * Returns a bitwise mask of a segment of bits, [index_begin, index_end).
 *
 * \tparam T          Type of the mask to be created.
 * \param index_begin Start of interval.
 * \param index_end   End of interval.
 */
template <typename T>
static inline T bit_mask(size_t index_begin, unsigned int index_end, _ENABLE_IF_UINT(T)) {
  return bit_mask<T>(index_begin) ^ bit_mask<T>(index_end);
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshift-count-overflow"
#endif
/**
 * Counts the number of bits on in x.
 *
 * \param v  Unsigned integer.
 */
template <typename T>
static inline unsigned int num_bits_on(T v, _ENABLE_IF_UINT(T))
{
  if(sizeof(unsigned int) >= sizeof(T))
    return __builtin_popcount((unsigned int)v);
  else if(sizeof(unsigned long) >= sizeof(T))
    return __builtin_popcountl((unsigned long)v);
  else if(sizeof(unsigned long long) >= sizeof(T))
    return __builtin_popcountll((unsigned long long)v);
  else if(bitsizeof(unsigned long long) == 64 && bitsizeof(T) == 128)
  // This errors with error: shift count >= width of type [-Werror,-Wshift-count-overflow]
  // but, at runtime, we've already checked for that in the else if condition.
    return (__builtin_popcountll((unsigned long long)(v))
            + __builtin_popcountll((unsigned long long)(uint64_t(uint128_t(v) >> 64))));
  else {
    unsigned int bitcount = 0;
    T vt = v;
    for(size_t i = 0; i < bitsizeof(T); i += bitsizeof(unsigned long long) ) {
      bitcount += __builtin_popcountll( (unsigned long long)(vt) );
      vt = T(uint128_t(vt) >> bitsizeof(unsigned long long));
    }

    return vt;
  }
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
template <int n, typename T>
static inline void __n_trailing_zeros_disect(
    T& v, int& c, _ENABLE_IF_BITSIZE_GT(T, n)) {

  if( T(v << (bitsizeof(T) - n)) == 0) {
    v >>= n;
    c += n;
  }
}

template <int n, typename T>
static inline void __n_trailing_zeros_disect(
    T& v, int& c, _ENABLE_IF_BITSIZE_LEQ(T, n)) {
}

/**
 * Counts the number of trailing zeros in v.  For example,
 * 010111011000 has 3 trailing zeros.  Returns bitsizeof(T) if v is
 * zero.
 *
 * \tparam T type of v.
 * \param v  Unsigned integer.
 */
template <typename T>
static inline unsigned int n_trailing_zeros(T v, _ENABLE_IF_UINT(T)) {
  if(!v) return bitsizeof(T);

  if(sizeof(unsigned int) >= sizeof(T))
    return __builtin_ctz((unsigned int)v);
  else if(sizeof(unsigned long) >= sizeof(T))
    return __builtin_ctzl((unsigned long)v);
  else if(sizeof(unsigned long long) >= sizeof(T))
    return __builtin_ctzll((unsigned long long)v);
  else if(bitsizeof(unsigned long long) == 64 && bitsizeof(T) == 128)
    return ((uint64_t(v) != 0)
            ? __builtin_ctzll((unsigned long long)v)
            : (64 + __builtin_ctzll((unsigned long long)(uint128_t(v) >> 64))));
  else
  {

    int c = 1;  // c will be the number of zero bits on the right,
    // so if v is 1101000 (base 2), then c will be 3

    __n_trailing_zeros_disect<128>(v, c);
    __n_trailing_zeros_disect<64>(v, c);
    __n_trailing_zeros_disect<32>(v, c);
    __n_trailing_zeros_disect<16>(v, c);
    __n_trailing_zeros_disect<8>(v, c);
    __n_trailing_zeros_disect<4>(v, c);
    __n_trailing_zeros_disect<2>(v, c);
    __n_trailing_zeros_disect<1>(v, c);

    c -= (v & 0x1);
    return c;
  }
}

/**
 * Counts the number of trailing ones in v.  For example,
 * 010111010111 has 3 trailing ones.  Returns bitsizeof(T) if v is
 * (~0).
 *
 * \tparam T type of v.
 * \param v  Unsigned integer.
 */
template <typename T>
static inline unsigned int n_trailing_ones(const T& v, _ENABLE_IF_UINT(T)) {
  return n_trailing_zeros(T(~v));
}

/**
 * Returns the index of the first on bit in v.  For example,
 * 010111011000 gives 3.  Returns bitsizeof(T) if v is zero.
 *
 * \tparam T type of v.
 * \param v  Unsigned integer.
 */
template <typename T>
static inline unsigned int index_first_on_bit(const T& v, _ENABLE_IF_UINT(T)) {
  return n_trailing_zeros(v);
}


////////////////////////////////////////////////////////////

template <int n, typename T>
static inline void __n_leading_zeros_disect(T& v, int& c, _ENABLE_IF_BITSIZE_GT(T, n)) {
  if((v >> (bitsizeof(T) - n)) == 0) {
    v <<= n; c += n;
  }
}

template <int n, typename T>
static inline void __n_leading_zeros_disect(T& v, int& c, _ENABLE_IF_BITSIZE_LEQ(T, n))
{}


/**
 * Counts the number of leading zeros in v.  For example, 01011000 has
 * 1 leading zero.  Returns bitsizeof(T) if v is zero.
 *
 * \tparam T type of v.
 * \param v  Unsigned integer value.
 */
template <typename T>
static inline unsigned int n_leading_zeros(T v, _ENABLE_IF_UINT(T)) {

  if(!v) return bitsizeof(T);


  if(sizeof(unsigned int) >= sizeof(T))
    return __builtin_clz((unsigned int)v) - (bitsizeof(unsigned int) - bitsizeof(T));
  else if(sizeof(unsigned long) >= sizeof(T))
    return __builtin_clzl((unsigned long)v)  - (bitsizeof(unsigned long) - bitsizeof(T));
  else if(sizeof(unsigned long long) >= sizeof(T))
    return __builtin_clzll((unsigned long long)v) - (bitsizeof(unsigned long long) - bitsizeof(T));
  else if(bitsizeof(unsigned long long) == 64 && bitsizeof(v) == 128)
    return (((uint128_t(v) >> 64) != 0)
            ? __builtin_clzll((unsigned long long)(uint128_t(v) >> 64))
            : (64 + __builtin_clzll((unsigned long long)(v))));
  else
  {
    int c = 0;
    // c will be the number of zero bits on the right,
    // so if v is 1101000 (base 2), then c will be 3

    __n_leading_zeros_disect<128>(v, c);
    __n_leading_zeros_disect<64>(v, c);
    __n_leading_zeros_disect<32>(v, c);
    __n_leading_zeros_disect<16>(v, c);
    __n_leading_zeros_disect<8>(v, c);
    __n_leading_zeros_disect<4>(v, c);
    __n_leading_zeros_disect<2>(v, c);
    __n_leading_zeros_disect<1>(v, c);

    // c += !(v >> (bitsizeof(T) - 1));

    return c;
  }
}

/**
 * Counts the number of leading ones in v.  For example, 11011000 has
 * 2 leading ones.  Returns bitsizeof(T) if v is (~0).
 *
 * \tparam T type of v.
 * \param v  Unsigned integer value.
 */
template <typename T>
static inline unsigned int n_leading_ones(T v, _ENABLE_IF_UINT(T)) {
  return n_leading_zeros(T(~v));
}


/**
 * Index of the last on bit. For example, 01101100 is 6.  Returns bitsizeof(T) if v is zero.
 *
 * \tparam T Type of v.
 * \param v  Unsigned integer value.
 */
template <typename T>
static inline unsigned int index_last_on_bit(const T& v, _ENABLE_IF_UINT(T)) {
  return bitsizeof(T) - (( (!v) - 1) & (1 + n_leading_zeros(v)));
}

/**
 * Returns the rounded up version of the bitwise log base two of the
 * number.  For example, 00010000 returns 4, and 00010001 returns 5.
 * If v is zero, zero is returned.
 *
 * \tparam T Type of v.
 * \param v  Unsigned integer value.
 */
template <typename T>
static inline unsigned int bitwise_log2_ceil(const T& v, _ENABLE_IF_UINT(T)) {
  return ( (!v) - 1) & (index_last_on_bit(v) + !is_power_of_2(v));
}

/**
 * Returns the rounded down version of the bitwise log base two of the
 * number.  For example, 00010000 returns 4, 00011111 returns 4, and
 * 00100000 returns 5.  If v is zero, zero is returned.
 *
 * \tparam T Type of v.
 * \param v  Unsigned integer value.
 */
template <typename T>
static inline unsigned int bitwise_log2_floor(const T& v, _ENABLE_IF_UINT(T)) {
  return ((!v) - 1) & (index_last_on_bit(v));
}

/**
 * Returns the modulus of v rounded to the pow2_idx bit. It is the
 * same as v % (2 ** pow2_idx).  For example, bitwise_pow2_mod(10, 3)
 * = 10 % 8 = 2.
 *
 * \tparam T Type of v.
 * \param v  Unsigned integer value.
 * \param pow2_idx  Power of 2 with which to take the mod.
 */
template <typename T>
static inline T bitwise_pow2_mod(const T& v, unsigned pow2_idx, _ENABLE_IF_UINT(T)) {
  return (!! pow2_idx) * bit_mask<T>(pow2_idx) & v;
}

/**
 * Returns true if the first n bits of v are on.
 *
 * \tparam T Type of v.
 * \param v  Unsigned integer value.
 */
template <typename T>
static inline bool first_n_bits_on(const T& v, unsigned int top_bit, _ENABLE_IF_UINT(T)) {
  return (v & bit_mask<T>(top_bit)) == bit_mask<T>(top_bit);
}

};

#endif
