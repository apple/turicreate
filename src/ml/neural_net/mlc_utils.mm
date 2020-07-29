/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/mlc_utils.hpp>

#include <functional>
#include <numeric>

namespace turi {
namespace neural_net {

// TODO: If we standardize on NCHW in the toolkit code, then we can avoid the
// allocation and copy here. Instead, we can create an NSData that wraps the
// shared_float_array.
NSData *convert_hwc_array_to_chw_data(const float_array &arr)
{
  // Allocate memory for the result, transferring ownership immediately to
  // NSData for exception safety.
  size_t size = arr.size() * sizeof(float);
  float *buffer = reinterpret_cast<float *>(malloc(size));
  NSData *data = [NSData dataWithBytesNoCopy:buffer length:size freeWhenDone:YES];

  // Copy and transpose.
  turi::neural_net::convert_hwc_to_chw(arr, buffer, buffer + arr.size());
  return data;
}

shared_float_array convert_chw_data_to_hwc_array(NSData *data, std::vector<size_t> hwc_shape)
{
  // First, wrap the NSData as a float_array with the NCHW shape from MLCompute. In the GPU case,
  // the NSData may be some page-aligned allocation larger than the tensor.
  std::vector<size_t> chw_shape = hwc_shape;
  auto last_three = chw_shape.end() - 3;
  std::rotate(last_three, last_three + 2, last_three + 3);  // HWC -> CHW
  size_t tensor_size =
      std::accumulate(chw_shape.begin(), chw_shape.end(), 1, std::multiplies<size_t>());
  assert(data.length >= tensor_size * sizeof(float));
  external_float_array chw_array(reinterpret_cast<const float *>(data.bytes), tensor_size,
                                 chw_shape.data(), chw_shape.size());

  // Copy from the NCHW NSData into a NHWC buffer.
  std::vector<float> hwc_data(chw_array.size());  // TODO: Avoid initializing
  turi::neural_net::convert_chw_to_hwc(chw_array, hwc_data.data(),
                                       hwc_data.data() + hwc_data.size());

  // Wrap the NHWC buffer and return it.
  return shared_float_array::wrap(std::move(hwc_data), std::move(hwc_shape));
}

// TODO: Investigate the number of copies a tensor endures end-to-end.
NSData *copy_data(const float_array &arr)
{
  NSUInteger length = arr.size() * sizeof(float);
  return [NSData dataWithBytes:arr.data() length:length];
}

}  // neural_net
}  // turi
