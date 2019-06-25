/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/generics/gl_string.hpp>

#ifndef TURI_STRING_CONVERSION_INTERNALS_H_
#define TURI_STRING_CONVERSION_INTERNALS_H_

namespace turi { namespace gl_string_internal {

// as_string
template <typename V>
GL_HOT_INLINE_FLATTEN static inline
gl_string as_string(const char* fmt, V a) {
  gl_string s;
  s.resize(s.capacity());
  size_t available = s.size();
  while (true) {
    int status = snprintf(&s[0], available, fmt, a);
    if ( status >= 0 ) {
      size_t used = static_cast<size_t>(status);
      if ( used <= available ) {
        s.resize( used );
        break;
      }
      available = used; // Assume this is advice of how much space we need.
    } else {
      available = available * 2 + 1;
    }

    s.resize(available);
  }
  return s;
}

}}

#endif /* _STRING_CONVERSION_INTERNALS_H_ */
