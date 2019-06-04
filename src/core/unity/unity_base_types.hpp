/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_BASE_TYPES_HPP
#define TURI_UNITY_BASE_TYPES_HPP
#include <memory>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sarray.hpp>
namespace turi {

/// The sframe type
typedef sframe sframe_type;

typedef sarray<flexible_type> sarray_type;

} // namespace turi

#endif
