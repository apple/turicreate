/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_BASIC_TYPES_H_
#define TURI_BASIC_TYPES_H_

#include <core/logging/assertions.hpp>
#include <cstdint>


template<typename Target, typename Source, int UnusedParam> struct truncate_check_impl {
  Target operator()(Source x);
};

template<typename Target, typename Source> Target truncate_check(Source x) {
  return truncate_check_impl<Target, Source, 0>()(x);
}

template<typename Target, typename Source> struct truncate_check_impl<Target, Source, 0> {
  Target operator()(Source x) {
    static_assert(!std::is_same<Source, Source>::value, "Unknown instantiation of truncate_check");
    return 0;
  }
};

template<> struct truncate_check_impl<int64_t, size_t, 0> {
  int64_t operator()(size_t x) {
    ASSERT_LT(x, (1ULL << 63));
    return static_cast<int64_t>(x);
  }
};

// For integer values, returns ceil(n / m);
template<typename T> T ceil_divide(T n, T m) {
  return (n + (m-1)) / m;
}

#endif
