/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <vector>

#include <core/util/Span.hpp>
#include <ml/neural_net/float16.h>

namespace turi {
namespace neural_net {

#ifdef TURI_USE_FLOAT16

bool is_convertible_to_fp16(Span<const float> output);

std::vector<__fp16> get_half_precision_weights(Span<const float> output);

#endif

}  // namespace neural_net
}  // namespace turi
