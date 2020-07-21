/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#import <MLCompute/MLCompute.h>

#include <ml/neural_net/mps_utils.h>

namespace turi {
namespace neural_net {

NSData *convert_hwc_array_to_chw_data(const float_array &arr);

shared_float_array convert_chw_data_to_hwc_array(NSData *data, std::vector<size_t> hwc_shape);

NSData *copy_data(const float_array &arr);

}  // namespace neural_net
}  // namespace turi
