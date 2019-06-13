/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UTIL_LOCK_FREE_INTERNAL_HPP
#define TURI_UTIL_LOCK_FREE_INTERNAL_HPP

#include <core/generics/integer_selector.hpp>

namespace turi {
namespace lock_free_internal {

template <typename index_type>
union reference_with_counter {
  struct {
    index_type val;
    index_type counter;
  } q;
  index_type& value() {
    return q.val;
  }
  index_type& counter() {
    return q.counter;
  }
  typename u_integer_selector<sizeof(index_type) * 2>::integer_type combined;
};

}
}
#endif
