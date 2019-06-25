/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef SERIALIZABLE_POD_HPP
#define SERIALIZABLE_POD_HPP

#include <core/storage/serialization/is_pod.hpp>

#define SERIALIZABLE_POD(tname)                   \
namespace turi {                              \
    template <>                                   \
    struct gl_is_pod<tname> {                     \
      BOOST_STATIC_CONSTANT(bool, value = true);  \
    };                                            \
}

#endif
