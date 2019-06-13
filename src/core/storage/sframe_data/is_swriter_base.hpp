/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_IS_SWRITER_BASE_HPP
#define TURI_UNITY_IS_SWRITER_BASE_HPP


#include <iterator>
#include <type_traits>
#include <core/storage/sframe_data/swriter_base.hpp>
namespace turi {
namespace sframe_impl {


/**
 * \ingroup sframe_physical
 * is_swriter_base<T>::value is true if T inherits from swriter_base
 */
template <typename T,
          typename DecayedT = typename std::decay<T>::type,
          typename Iterator = typename DecayedT::iterator>
struct is_swriter_base {
  static constexpr bool value =
      std::is_base_of<turi::swriter_base<Iterator>, DecayedT>::value;
};


template <typename T>
struct is_swriter_base<T,void,void> {
  static constexpr bool value = false;
};



} // sframe_impl
} // turicreate

#endif
