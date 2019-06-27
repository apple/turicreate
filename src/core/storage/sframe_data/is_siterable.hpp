/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_IS_SITERABLE_HPP
#define TURI_UNITY_IS_SITERABLE_HPP

#include <iterator>
#include <type_traits>
#include <core/storage/sframe_data/siterable.hpp>
namespace turi {
namespace sframe_impl {


/**
 * \ingroup sframe_physical
 * is_siterable<T>::value is true if T inherits from siterable
 */
template <typename T,
          typename DecayedT = typename std::decay<T>::type,
          typename Iterator = typename DecayedT::iterator>
struct is_siterable {
  static constexpr bool value =
      std::is_base_of<turi::siterable<Iterator>, DecayedT>::value;
};


template <typename T>
struct is_siterable<T,void,void> {
  static constexpr bool value = false;
};



} // sframe_impl
} // turicreate

#endif
