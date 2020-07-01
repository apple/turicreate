/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/quantization_utils.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace turi {
namespace neural_net {

#ifdef TURI_USE_FLOAT16

bool is_convertible_to_fp16(Span<const float> output)
{
  return std::all_of(output.begin(), output.end(), [](int i) {
    return i > -FLOAT16_NUMERIC_LIMIT_MAX && i < FLOAT16_NUMERIC_LIMIT_MAX;
  });
}

std::vector<__fp16> get_half_precision_weights(Span<const float> output)
{
  std::vector<__fp16> weights_fp16 = std::vector<__fp16>(output.begin(), output.end());
  return weights_fp16;
}

#endif

}  // namespace neural_net
}  // namespace turi
