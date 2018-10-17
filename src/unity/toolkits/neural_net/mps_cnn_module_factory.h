/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_MPS_CNN_MODULE_FACTORY_H_
#define UNITY_TOOLKITS_NEURAL_NET_MPS_CNN_MODULE_FACTORY_H_

#include <unity/toolkits/neural_net/cnn_module.hpp>

namespace turi {
namespace neural_net {

std::unique_ptr<cnn_module> create_mps_object_detector(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights);

}  // namespace neural_net
}  // namespace turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_MPS_CNN_MODULE_FACTORY_H_
