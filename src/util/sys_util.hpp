/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_SYS_UTIL_H_
#define TURI_SYS_UTIL_H_

#if defined(_MSC_VER)
  #define TURI_BUILTIN_UNREACHABLE()
  #define TURI_ATTRIBUTE_UNUSED
  #define TURI_ATTRIBUTE_UNUSED_NDEBUG
#elif defined(__GNUC__)
  #define TURI_BUILTIN_UNREACHABLE() __builtin_unreachable()
  #define TURI_ATTRIBUTE_UNUSED __attribute__((unused))
  #ifdef NDEBUG
    #define TURI_ATTRIBUTE_UNUSED_NDEBUG TURI_ATTRIBUTE_UNUSED
  #else
    #define TURI_ATTRIBUTE_UNUSED_NDEBUG
  #endif
#elif defined(__clang__)
  #define TURI_BUILTIN_UNREACHABLE() __builtin_unreachable()
  #define TURI_ATTRIBUTE_UNUSED __attribute__((unused))
  #ifdef NDEBUG
    #define TURI_ATTRIBUTE_UNUSED_NDEBUG TURI_ATTRIBUTE_UNUSED
  #else
    #define TURI_ATTRIBUTE_UNUSED_NDEBUG
  #endif
#else
  #error Unrecognized compiler platform.
#endif  // Platform

#endif
