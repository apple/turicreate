/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <ml/neural_net/float_array.hpp>
#include <toolkits/style_transfer/style_transfer_model_definition.hpp>

#include "utils.hpp"

using namespace turi::style_transfer;
using namespace turi::neural_net;

BOOST_AUTO_TEST_CASE(test_load_vgg_16) {
  /**
   * TODO: Delete this path once uploaded to https://developer.apple.com/
   **/
  std::string VGG_16_MODEL_PATH =
      "/Users/abhishekpratapa/Desktop/mxnet_golden_set/vgg16.mlmodel";

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

  for (const auto &weight : weights)
    TS_ASSERT(expected_keys.count(weight.first) != 0);
}

BOOST_AUTO_TEST_CASE(test_load_resnet) {
  /**
   * TODO: Delete this path once uploaded to https://developer.apple.com/
   **/
  std::string RESNET_MODEL_PATH =
      "/Users/abhishekpratapa/Desktop/mxnet_golden_set/transformer.mlmodel";

  std::set<std::string> expected_keys{
      "transformer_conv5_weight",
      "transformer_decoding_1_conv_weight",
      "transformer_decoding_1_inst_beta_weight",
      "transformer_decoding_1_inst_gamma_weight",
      "transformer_decoding_2_conv_weight",
      "transformer_decoding_2_inst_beta_weight",
      "transformer_decoding_2_inst_gamma_weight",
      "transformer_encode_1_conv_weight",
      "transformer_encode_1_inst_beta_weight",
      "transformer_encode_1_inst_gamma_weight",
      "transformer_encode_2_conv_weight",
      "transformer_encode_2_inst_beta_weight",
      "transformer_encode_2_inst_gamma_weight",
      "transformer_encode_3_conv_weight",
      "transformer_encode_3_inst_beta_weight",
      "transformer_encode_3_inst_gamma_weight",
      "transformer_instancenorm0__fwd_normalize_beta",
      "transformer_instancenorm0__fwd_normalize_gamma",
      "transformer_instancenorm1__fwd_normalize_beta",
      "transformer_instancenorm1__fwd_normalize_gamma",
      "transformer_instancenorm2__fwd_normalize_beta",
      "transformer_instancenorm2__fwd_normalize_gamma",
      "transformer_instancenorm3__fwd_normalize_beta",
      "transformer_instancenorm3__fwd_normalize_gamma",
      "transformer_instancenorm4__fwd_normalize_beta",
      "transformer_instancenorm4__fwd_normalize_gamma",
      "transformer_instancenorm5__fwd_normalize_beta",
      "transformer_instancenorm5__fwd_normalize_gamma",
      "transformer_instancenorm5_beta_weight",
      "transformer_instancenorm5_gamma_weight",
      "transformer_residual_1_conv_1_weight",
      "transformer_residual_1_conv_2_weight",
      "transformer_residual_1_inst_1_beta_weight",
      "transformer_residual_1_inst_1_gamma_weight",
      "transformer_residual_1_inst_2_beta_weight",
      "transformer_residual_1_inst_2_gamma_weight",
      "transformer_residual_2_conv_1_weight",
      "transformer_residual_2_conv_2_weight",
      "transformer_residual_2_inst_1_beta_weight",
      "transformer_residual_2_inst_1_gamma_weight",
      "transformer_residual_2_inst_2_beta_weight",
      "transformer_residual_2_inst_2_gamma_weight",
      "transformer_residual_3_conv_1_weight",
      "transformer_residual_3_conv_2_weight",
      "transformer_residual_3_inst_1_beta_weight",
      "transformer_residual_3_inst_1_gamma_weight",
      "transformer_residual_3_inst_2_beta_weight",
      "transformer_residual_3_inst_2_gamma_weight",
      "transformer_residual_4_conv_1_weight",
      "transformer_residual_4_conv_2_weight",
      "transformer_residual_4_inst_1_beta_weight",
      "transformer_residual_4_inst_1_gamma_weight",
      "transformer_residual_4_inst_2_beta_weight",
      "transformer_residual_4_inst_2_gamma_weight",
      "transformer_residual_5_conv_1_weight",
      "transformer_residual_5_conv_2_weight",
      "transformer_residual_5_inst_1_beta_weight",
      "transformer_residual_5_inst_1_gamma_weight",
      "transformer_residual_5_inst_2_beta_weight",
      "transformer_residual_5_inst_2_gamma_weight",
      "transformer_residualblock0_instancenorm0__fwd_normalize_beta",
      "transformer_residualblock0_instancenorm0__fwd_normalize_gamma",
      "transformer_residualblock0_instancenorm1__fwd_normalize_beta",
      "transformer_residualblock0_instancenorm1__fwd_normalize_gamma",
      "transformer_residualblock1_instancenorm0__fwd_normalize_beta",
      "transformer_residualblock1_instancenorm0__fwd_normalize_gamma",
      "transformer_residualblock1_instancenorm1__fwd_normalize_beta",
      "transformer_residualblock1_instancenorm1__fwd_normalize_gamma",
      "transformer_residualblock2_instancenorm0__fwd_normalize_beta",
      "transformer_residualblock2_instancenorm0__fwd_normalize_gamma",
      "transformer_residualblock2_instancenorm1__fwd_normalize_beta",
      "transformer_residualblock2_instancenorm1__fwd_normalize_gamma",
      "transformer_residualblock3_instancenorm0__fwd_normalize_beta",
      "transformer_residualblock3_instancenorm0__fwd_normalize_gamma",
      "transformer_residualblock3_instancenorm1__fwd_normalize_beta",
      "transformer_residualblock3_instancenorm1__fwd_normalize_gamma",
      "transformer_residualblock4_instancenorm0__fwd_normalize_beta",
      "transformer_residualblock4_instancenorm0__fwd_normalize_gamma",
      "transformer_residualblock4_instancenorm1__fwd_normalize_beta",
      "transformer_residualblock4_instancenorm1__fwd_normalize_gamma"};

  std::unique_ptr<model_spec> nn_spec = init_resnet(RESNET_MODEL_PATH, 8);
  float_array_map weights = nn_spec->export_params_view();

  for (const auto &weight : weights)
    TS_ASSERT(expected_keys.count(weight.first) != 0);
}