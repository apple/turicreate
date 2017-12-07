/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_GENERICS_IS_MEMMOVABLE_HPP
#define TURI_GENERICS_IS_MEMMOVABLE_HPP
#include <type_traits>
namespace turi {

template <typename T>
struct is_memmovable {
  static constexpr bool value = std::is_trivially_copyable<T>::value;
};
};

#endif
