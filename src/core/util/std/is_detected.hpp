/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#if __cplusplus > 201103L

#include <experimental/type_traits>

#else

// this is a C++11 implementation for std::experimental::detail (see online cppreference
// for full details).
//
// this enables static interface testing for the presence of functions and methods.
//
// impl reference: https://people.eecs.berkeley.edu/~brock/blog/detection_idiom.php

namespace std {

template <class...>
using void_t = void;

namespace experimental {
struct nonesuch {
  ~nonesuch() = delete;
  nonesuch(nonesuch const&) = delete;
  void operator=(nonesuch const&) = delete;
};

namespace detail {
template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
struct detector {
  using value_t = std::false_type;
  using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...> >, Op, Args...> {
  using value_t = std::true_type;
  using type = Op<Args...>;
};

}  // namespace detail

template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

}  // namespace experimental
}  // namespace std

#endif
