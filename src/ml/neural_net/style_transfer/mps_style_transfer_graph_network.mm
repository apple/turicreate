/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer_graph_network.hpp>

/* DEBUG */
#import <iostream>
/* DEBUG */

namespace turi {
namespace neural_net {

STNetworkLayer::STNetworkLayer(const std::string &layer_name,
                               const std::vector<int> &i_shape,
                               const std::vector<int> &o_shape) {
  name = layer_name;
  ishape = i_shape;
  oshape = o_shape;
}

void STNetworkLayer::Init(id<MTLDevice> device, id<MTLCommandQueue> cmd_queue,
                          const float_array_map &config,
                          const float_array_map &weights) {

  int num_styles = (int) get_array_map_scalar(config, "st_num_styles", 1);
  NSMutableDictionary<NSString *, NSData *> *styleTransferWeights = [[NSMutableDictionary alloc] init];

  std::map<std::string, NSString *> MPSStyleTransferKeyMap = {
      {"transformer_conv0_weight", @"transformer_encode_1_conv_weights"},
      {"transformer_conv1_weight", @"transformer_encode_2_conv_weights"},
      {"transformer_conv2_weight", @"transformer_encode_3_conv_weights"},
      {"transformer_conv3_weight", @"transformer_decoding_1_conv_weights"},
      {"transformer_conv4_weight", @"transformer_decoding_2_conv_weights"},
      {"transformer_conv5_weight", @"transformer_conv5_weight"},
      {"transformer_instancenorm0_beta", @"transformer_encode_1_inst_beta"},
      {"transformer_instancenorm0_gamma", @"transformer_encode_1_inst_gamma"},
      {"transformer_instancenorm1_beta", @"transformer_encode_2_inst_beta"},
      {"transformer_instancenorm1_gamma", @"transformer_encode_2_inst_gamma"},
      {"transformer_instancenorm2_beta", @"transformer_encode_3_inst_beta"},
      {"transformer_instancenorm2_gamma", @"transformer_encode_3_inst_gamma"},
      {"transformer_instancenorm3_beta", @"transformer_decoding_1_inst_beta"},
      {"transformer_instancenorm3_gamma", @"transformer_decoding_1_inst_gamma"},
      {"transformer_instancenorm4_beta", @"transformer_decoding_2_inst_beta"},
      {"transformer_instancenorm4_gamma", @"transformer_decoding_2_inst_gamma"},
      {"transformer_instancenorm5_beta", @"transformer_instancenorm5_gamma"},
      {"transformer_instancenorm5_gamma", @"transformer_instancenorm5_beta"},
      {"transformer_residualblock0_conv0_weight", @"transformer_residual_1_conv_1_weights"},
      {"transformer_residualblock0_conv1_weight", @"transformer_residual_1_conv_2_weights"},
      {"transformer_residualblock0_instancenorm0_beta", @"transformer_residual_1_inst_1_beta"},
      {"transformer_residualblock0_instancenorm0_gamma", @"transformer_residual_1_inst_1_gamma"},
      {"transformer_residualblock0_instancenorm1_beta", @"transformer_residual_1_inst_2_beta"},
      {"transformer_residualblock0_instancenorm1_gamma", @"transformer_residual_1_inst_2_gamma"},
      {"transformer_residualblock1_conv0_weight", @"transformer_residual_2_conv_1_weights"},
      {"transformer_residualblock1_conv1_weight", @"transformer_residual_2_conv_2_weights"},
      {"transformer_residualblock1_instancenorm0_beta", @"transformer_residual_2_inst_1_beta"},
      {"transformer_residualblock1_instancenorm0_gamma", @"transformer_residual_2_inst_1_gamma"},
      {"transformer_residualblock1_instancenorm1_beta", @"transformer_residual_2_inst_2_beta"},
      {"transformer_residualblock1_instancenorm1_gamma", @"transformer_residual_2_inst_2_gamma"},
      {"transformer_residualblock2_conv0_weight", @"transformer_residual_3_conv_1_weights"},
      {"transformer_residualblock2_conv1_weight", @"transformer_residual_3_conv_2_weights"},
      {"transformer_residualblock2_instancenorm0_beta", @"transformer_residual_3_inst_1_beta"},
      {"transformer_residualblock2_instancenorm0_gamma", @"transformer_residual_3_inst_1_gamma"},
      {"transformer_residualblock2_instancenorm1_beta", @"transformer_residual_3_inst_2_beta"},
      {"transformer_residualblock2_instancenorm1_gamma", @"transformer_residual_3_inst_2_gamma"},
      {"transformer_residualblock3_conv0_weight", @"transformer_residual_4_conv_1_weights"},
      {"transformer_residualblock3_conv1_weight", @"transformer_residual_4_conv_2_weights"},
      {"transformer_residualblock3_instancenorm0_beta", @"transformer_residual_4_inst_1_beta"},
      {"transformer_residualblock3_instancenorm0_gamma", @"transformer_residual_4_inst_1_gamma"},
      {"transformer_residualblock3_instancenorm1_beta", @"transformer_residual_4_inst_2_beta"},
      {"transformer_residualblock3_instancenorm1_gamma", @"transformer_residual_4_inst_2_gamma"},
      {"transformer_residualblock4_conv0_weight", @"transformer_residual_5_conv_1_weights"},
      {"transformer_residualblock4_conv1_weight", @"transformer_residual_5_conv_2_weights"},
      {"transformer_residualblock4_instancenorm0_beta", @"transformer_residual_5_inst_1_beta"},
      {"transformer_residualblock4_instancenorm0_gamma", @"transformer_residual_5_inst_1_gamma"},
      {"transformer_residualblock4_instancenorm1_beta", @"transformer_residual_5_inst_2_beta"},
      {"transformer_residualblock4_instancenorm1_gamma", @"transformer_residual_5_inst_2_gamma"}};

  for (const auto &weight : weights) {
    const float* weightData = weight.second.data();
    const size_t weightSize = weight.second.size() * sizeof(float);

    NSMutableData * mutableWeightData = [NSMutableData data];
    [mutableWeightData appendBytes:&weightData length:weightSize];

    NSString *weightKey = MPSStyleTransferKeyMap[weight.first];
    styleTransferWeights[weightKey] = [NSData dataWithData:mutableWeightData];
  }

  network = [[TCMPSStyleTransfer alloc] initWithDev:device
                                       commandQueue:cmd_queue
                                            weights:[styleTransferWeights copy]
                                          numStyles:(NSUInteger)num_styles];
}

void STNetworkLayer::InitFwd(MPSNNImageNode *src) {}

void STNetworkLayer::InitBwd(MPSNNImageNode *src) {}

void STNetworkLayer::SetLearningRate(float lr) {}

float_array_map STNetworkLayer::Export() const { return float_array_map(); }

STNetworkGraph::STNetworkGraph(const std::vector<int> &iparam,
                               const float_array_map &config) {
  int batch_size = iparam[0];

  int height_input = iparam[1];
  int width_input = iparam[2];
  int channel_input = iparam[3];

  int height_output = iparam[4];
  int width_output = iparam[5];
  int channel_output = iparam[6];

  bool include_network = get_array_map_bool(config, "st_include_network", true);
  bool include_loss = get_array_map_bool(config, "st_include_loss", true);

  if (include_network) {
    layers.push_back(new STNetworkLayer(
        "tcmps_style_transfer_network",
        {batch_size, height_input, width_input, channel_input},
        {batch_size, height_output, width_output, channel_output}));
  }

  if (include_loss) {
    // loss_layer_.reset()
  }
}

} // namespace neural_net
} // namespace turi