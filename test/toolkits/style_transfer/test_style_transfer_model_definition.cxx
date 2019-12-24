/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE

#include <map>
#include <memory>
#include <set>
#include <string>

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <ml/neural_net/float_array.hpp>
#include <ml/coreml_export/mlmodel_include.hpp>
#include <toolkits/style_transfer/style_transfer_model_definition.hpp>

#include "utils.hpp"

using CoreML::Specification::InnerProductLayerParams;
using CoreML::Specification::NeuralNetwork;
using CoreML::Specification::NeuralNetworkLayer;

using namespace turi::style_transfer;
using namespace turi::neural_net;

BOOST_AUTO_TEST_CASE(test_load_vgg_16) {
  std::string VGG_16_MODEL_PATH = get_vgg16_model();

  std::set<std::string> expected_keys{
      "vgg_block_1_conv_1_bias", "vgg_block_1_conv_1_weight",
      "vgg_block_1_conv_2_bias", "vgg_block_1_conv_2_weight",
      "vgg_block_2_conv_1_bias", "vgg_block_2_conv_1_weight",
      "vgg_block_2_conv_2_bias", "vgg_block_2_conv_2_weight",
      "vgg_block_3_conv_1_bias", "vgg_block_3_conv_1_weight",
      "vgg_block_3_conv_2_bias", "vgg_block_3_conv_2_weight",
      "vgg_block_3_conv_3_bias", "vgg_block_3_conv_3_weight",
      "vgg_block_4_conv_1_bias", "vgg_block_4_conv_1_weight",
      "vgg_block_4_conv_2_bias", "vgg_block_4_conv_2_weight",
      "vgg_block_4_conv_3_bias", "vgg_block_4_conv_3_weight"};

  std::unique_ptr<model_spec> nn_spec = init_vgg_16(VGG_16_MODEL_PATH);
  float_array_map weights = nn_spec->export_params_view();

  for (const auto& weight : weights)
    TS_ASSERT(expected_keys.count(weight.first) != 0);
}

BOOST_AUTO_TEST_CASE(test_load_resnet) {
  std::string RESNET_MODEL_PATH = get_resnet_model();

  std::set<std::string> expected_keys{
      "transformer_conv5_weight",
      "transformer_decoding_1_conv_weight",
      "transformer_decoding_1_conv_bias",
      "transformer_decoding_1_inst_beta_weight",
      "transformer_decoding_1_inst_beta_bias",
      "transformer_decoding_1_inst_gamma_weight",
      "transformer_decoding_1_inst_gamma_bias",
      "transformer_decoding_2_conv_weight",
      "transformer_decoding_2_conv_bias",
      "transformer_decoding_2_inst_beta_weight",
      "transformer_decoding_2_inst_beta_bias",
      "transformer_decoding_2_inst_gamma_weight",
      "transformer_decoding_2_inst_gamma_bias",
      "transformer_encode_1_conv_weight",
      "transformer_encode_1_conv_bias",
      "transformer_encode_1_inst_beta_weight",
      "transformer_encode_1_inst_beta_bias",
      "transformer_encode_1_inst_gamma_weight",
      "transformer_encode_1_inst_gamma_bias",
      "transformer_encode_2_conv_weight",
      "transformer_encode_2_conv_bias",
      "transformer_encode_2_inst_beta_weight",
      "transformer_encode_2_inst_beta_bias",
      "transformer_encode_2_inst_gamma_weight",
      "transformer_encode_2_inst_gamma_bias",
      "transformer_encode_3_conv_weight",
      "transformer_encode_3_conv_bias",
      "transformer_encode_3_inst_beta_weight",
      "transformer_encode_3_inst_beta_bias",
      "transformer_encode_3_inst_gamma_weight",
      "transformer_encode_3_inst_gamma_bias",
      "transformer_instancenorm0__fwd_bn__beta",
      "transformer_instancenorm0__fwd_bn__gamma",
      "transformer_instancenorm1__fwd_bn__beta",
      "transformer_instancenorm1__fwd_bn__gamma",
      "transformer_instancenorm2__fwd_bn__beta",
      "transformer_instancenorm2__fwd_bn__gamma",
      "transformer_instancenorm3__fwd_bn__beta",
      "transformer_instancenorm3__fwd_bn__gamma",
      "transformer_instancenorm4__fwd_bn__beta",
      "transformer_instancenorm4__fwd_bn__gamma",
      "transformer_instancenorm5__fwd_bn__beta",
      "transformer_instancenorm5__fwd_bn__gamma",
      "transformer_instancenorm5_beta_weight",
      "transformer_instancenorm5_beta_bias",
      "transformer_instancenorm5_gamma_weight",
      "transformer_instancenorm5_gamma_bias",
      "transformer_residual_1_conv_1_weight",
      "transformer_residual_1_conv_1_bias",
      "transformer_residual_1_conv_2_weight",
      "transformer_residual_1_conv_2_bias",
      "transformer_residual_1_inst_1_beta_weight",
      "transformer_residual_1_inst_1_beta_bias",
      "transformer_residual_1_inst_1_gamma_weight",
      "transformer_residual_1_inst_1_gamma_bias",
      "transformer_residual_1_inst_2_beta_weight",
      "transformer_residual_1_inst_2_beta_bias",
      "transformer_residual_1_inst_2_gamma_weight",
      "transformer_residual_1_inst_2_gamma_bias",
      "transformer_residual_2_conv_1_weight",
      "transformer_residual_2_conv_1_bias",
      "transformer_residual_2_conv_2_weight",
      "transformer_residual_2_conv_2_bias",
      "transformer_residual_2_inst_1_beta_weight",
      "transformer_residual_2_inst_1_beta_bias",
      "transformer_residual_2_inst_1_gamma_weight",
      "transformer_residual_2_inst_1_gamma_bias",
      "transformer_residual_2_inst_2_beta_weight",
      "transformer_residual_2_inst_2_beta_bias",
      "transformer_residual_2_inst_2_gamma_weight",
      "transformer_residual_2_inst_2_gamma_bias",
      "transformer_residual_3_conv_1_weight",
      "transformer_residual_3_conv_1_bias",
      "transformer_residual_3_conv_2_weight",
      "transformer_residual_3_conv_2_bias",
      "transformer_residual_3_inst_1_beta_weight",
      "transformer_residual_3_inst_1_beta_bias",
      "transformer_residual_3_inst_1_gamma_weight",
      "transformer_residual_3_inst_1_gamma_bias",
      "transformer_residual_3_inst_2_beta_weight",
      "transformer_residual_3_inst_2_beta_bias",
      "transformer_residual_3_inst_2_gamma_weight",
      "transformer_residual_3_inst_2_gamma_bias",
      "transformer_residual_4_conv_1_weight",
      "transformer_residual_4_conv_1_bias",
      "transformer_residual_4_conv_2_weight",
      "transformer_residual_4_conv_2_bias",
      "transformer_residual_4_inst_1_beta_weight",
      "transformer_residual_4_inst_1_beta_bias",
      "transformer_residual_4_inst_1_gamma_weight",
      "transformer_residual_4_inst_1_gamma_bias",
      "transformer_residual_4_inst_2_beta_weight",
      "transformer_residual_4_inst_2_beta_bias",
      "transformer_residual_4_inst_2_gamma_weight",
      "transformer_residual_4_inst_2_gamma_bias",
      "transformer_residual_5_conv_1_weight",
      "transformer_residual_5_conv_1_bias",
      "transformer_residual_5_conv_2_weight",
      "transformer_residual_5_conv_2_bias",
      "transformer_residual_5_inst_1_beta_weight",
      "transformer_residual_5_inst_1_beta_bias",
      "transformer_residual_5_inst_1_gamma_weight",
      "transformer_residual_5_inst_1_gamma_bias",
      "transformer_residual_5_inst_2_beta_weight",
      "transformer_residual_5_inst_2_beta_bias",
      "transformer_residual_5_inst_2_gamma_weight",
      "transformer_residual_5_inst_2_gamma_bias",
      "transformer_residualblock0_instancenorm0__fwd_bn__beta",
      "transformer_residualblock0_instancenorm0__fwd_bn__gamma",
      "transformer_residualblock0_instancenorm1__fwd_bn__beta",
      "transformer_residualblock0_instancenorm1__fwd_bn__gamma",
      "transformer_residualblock1_instancenorm0__fwd_bn__beta",
      "transformer_residualblock1_instancenorm0__fwd_bn__gamma",
      "transformer_residualblock1_instancenorm1__fwd_bn__beta",
      "transformer_residualblock1_instancenorm1__fwd_bn__gamma",
      "transformer_residualblock2_instancenorm0__fwd_bn__beta",
      "transformer_residualblock2_instancenorm0__fwd_bn__gamma",
      "transformer_residualblock2_instancenorm1__fwd_bn__beta",
      "transformer_residualblock2_instancenorm1__fwd_bn__gamma",
      "transformer_residualblock3_instancenorm0__fwd_bn__beta",
      "transformer_residualblock3_instancenorm0__fwd_bn__gamma",
      "transformer_residualblock3_instancenorm1__fwd_bn__beta",
      "transformer_residualblock3_instancenorm1__fwd_bn__gamma",
      "transformer_residualblock4_instancenorm0__fwd_bn__beta",
      "transformer_residualblock4_instancenorm0__fwd_bn__gamma",
      "transformer_residualblock4_instancenorm1__fwd_bn__beta",
      "transformer_residualblock4_instancenorm1__fwd_bn__gamma"};

  std::unique_ptr<model_spec> nn_spec = init_resnet(RESNET_MODEL_PATH);
  float_array_map weights = nn_spec->export_params_view();

  for (const auto& weight : weights)
    TS_ASSERT(expected_keys.count(weight.first) != 0);
}

BOOST_AUTO_TEST_CASE(test_update_num_styles) {
  std::string RESNET_MODEL_PATH = get_resnet_model();

  size_t NUM_STYLES = 8;

  std::unique_ptr<model_spec> nn_spec =
      init_resnet(RESNET_MODEL_PATH, NUM_STYLES);

  CoreML::Specification::NeuralNetwork neural_net = nn_spec->get_coreml_spec();

  for (const NeuralNetworkLayer& layer : neural_net.layers()) {
    if (layer.name().find("_inst_") != std::string::npos) {
      const InnerProductLayerParams& params = layer.innerproduct();

      // Checking if num styles matches
      TS_ASSERT(params.inputchannels() == NUM_STYLES);

      // Checking if the actual_size of the weights matches the expected values
      const float* actual_weight_data = params.weights().floatvalue().data();
      size_t actual_weight_size = params.weights().floatvalue().size();
      size_t expected_weight_size =
          params.inputchannels() * params.outputchannels();

      TS_ASSERT(actual_weight_size == expected_weight_size);

      // Checking if the weight initalization are the expected values
      if (layer.name().find("gamma") != std::string::npos) {
        for (size_t x = 0; x < actual_weight_size; x++)
          TS_ASSERT(actual_weight_data[x] == 1.0f);
      } else {
        for (size_t x = 0; x < actual_weight_size; x++)
          TS_ASSERT(actual_weight_data[x] == 0.0f);
      }
    }
  }
}
