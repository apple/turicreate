/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_MPS_STYLE_TRANSFER_GRAPH_NETWORK_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_MPS_STYLE_TRANSFER_GRAPH_NETWORK_HPP_

#import <vector>

#import <ml/neural_net/style_transfer/mps_style_transfer.h>

#include <ml/neural_net/mps_graph_layers.h>
#include <ml/neural_net/mps_graph_networks.h>
#include <ml/neural_net/mps_utils.h>

NS_ASSUME_NONNULL_BEGIN

namespace turi {
namespace neural_net {

struct STNetworkLayer : public GraphLayer {
  explicit STNetworkLayer(const std::string &layer_name,
                          const std::vector<int> &i_shape,
                          const std::vector<int> &o_shape);

  void Init(id<MTLDevice> device, id<MTLCommandQueue> cmd_queue,
            const float_array_map &config,
            const float_array_map &weights) override;

  void InitFwd(MPSNNImageNode *src) override;
  void InitBwd(MPSNNImageNode *src) override;

  void SetLearningRate(float lr) override;
  float_array_map Export() const override;

  TCMPSStyleTransfer *network;
};

struct STNetworkGraph : public MPSGraphNetwork {
  STNetworkGraph(const std::vector<int> &iparam, const float_array_map &config);
};

} // namespace neural_net
} // namespace turi

NS_ASSUME_NONNULL_END

#endif