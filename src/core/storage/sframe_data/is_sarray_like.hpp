/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SFRAME_IS_SARRAY_LIKE_HPP
#define TURI_UNITY_SFRAME_IS_SARRAY_LIKE_HPP
#include <memory>
#include <core/storage/sframe_data/is_siterable.hpp>
#include <core/storage/sframe_data/is_swriter_base.hpp>
namespace turi {
namespace sframe_impl {

template <typename T>
struct has_get_reader_function {
  template <typename U, std::unique_ptr<typename U::reader_type> (U::*)(size_t) const> struct SFINAE {};
  template<typename U> static char Test(SFINAE<U, &U::get_reader>*);
  template<typename U> static int Test(...);
  static constexpr bool value = sizeof(Test<T>(0)) == sizeof(char);
};


/**
 * \ingroup sframe_physical
 * is_sarray_like<T>::value is true if T inherits from swriter_base and
 * has a get_reader() function implemented which returns an
 * std::unique_ptr<T::reader_type>
 */
template <typename T,
          typename DecayedT = typename std::decay<T>::type,
          typename Iterator = typename DecayedT::iterator>
struct is_sarray_like {
  static constexpr bool value = is_swriter_base<T>::value &&
      has_get_reader_function<DecayedT>::value;
};



template <typename T>
struct is_sarray_like<T,void,void> {
  static constexpr bool value = false;
};


} // sframe_impl
} // turicreate
#endif
