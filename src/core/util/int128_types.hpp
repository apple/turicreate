/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
/**
 * \file int128_types.hpp
 * Defines the int128_t and uint128_t type.
 * Goes through various compiler checks to find the existance of int128_t
 * and use it if available. Requires a particular CMake script be executed
 * on configure to check for the int128_t variants.
 *
 * Also defines ostream printing, std::is_scalar, and boost::is_scalar on the
 * int128_t, uint128_t types.
 */
#ifndef _INT128_TYPES_H_
#define _INT128_TYPES_H_
#include <core/storage/serialization/is_pod.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <type_traits>
#include <cstdint>
#include <ostream>
#include <sstream>

// not in turicreate source tree. try to use
// compiler predefines to find int128
#ifndef IN_TURI_SOURCE_TREE
#if __SIZEOF_INT128__ == 16
#define HAVE__int128_t
#define HAVE__uint128_t
#endif
#endif

/* These macros are set by CMake in the configuration process. */
#ifdef HAVE__int128_t
typedef __int128_t int128_t;
#elif defined(HAVEint128_t)
// We're okay
#elif defined(HAVE__int128)
typedef __int128 int128_t;
#elif defined(HAVEint128)
typedef int128 int128_t;
#else
#include <boost/multiprecision/cpp_int.hpp>
  typedef boost::multiprecision::int128_t int128_t;
#endif

#ifdef HAVE__uint128_t
typedef __uint128_t uint128_t;
#elif defined(HAVEuint128_t)
// We're okay
#elif defined(HAVE__uint128)
typedef __uint128 uint128_t;
#elif defined(HAVEuint128)
typedef uint128 uint128_t;
#elif defined(HAVEunsigned__int128_t)
typedef unsigned __int128_t uint128_t;
#elif defined(HAVEunsignedint128_t)
typedef unsigned int128_t uint128_t;
#elif defined(HAVEunsigned__int128)
typedef unsigned __int128 uint128_t;
#elif defined(HAVEunsignedint128)
typedef unsigned int128 uint128_t;
#else
#include <boost/multiprecision/cpp_int.hpp>
  typedef boost::multiprecision::uint128_t uint128_t;
#endif

static inline uint128_t BuildUint128(uint64_t high, uint64_t low) {
  return ((uint128_t(high) << 64) + low);
}

/// Enables printing of uint128_t values
static inline std::ostream& operator<<(std::ostream& out, const uint128_t& x) {
  std::ostringstream s;
  s << std::hex << (uint64_t(x >> 64)) << uint64_t(x);
  out << s.str();
  return out;
}

namespace boost {
template <>
struct is_scalar<uint128_t> {
  static const bool value = true;
};

template <>
struct is_scalar<int128_t> {
  static const bool value = true;
};

} // boost



namespace std {
template <>
struct is_scalar<uint128_t> {
  static const bool value = true;
};

template <>
struct is_scalar<int128_t> {
  static const bool value = true;
};
} // std


#endif /* _INT128_TYPES_H_ */
