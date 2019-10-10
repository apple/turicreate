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

#include <iostream>

#include "utils.hpp"

using namespace turi::style_transfer;
using namespace turi::neural_net;

// BOOST_AUTO_TEST_CASE(test_load_vgg_16) {
//   /**
//    * TODO: Delete this path once uploaded to https://developer.apple.com/
//    **/
//   std::string VGG_16_MODEL_PATH =
//       "/Users/abhishekpratapa/Desktop/mxnet_golden_set/vgg16.mlmodel";

//   std::set<std::string> expected_keys{
//       "vgg_block_1_conv_1_bias", "vgg_block_1_conv_1_weight",
//       "vgg_block_1_conv_2_bias", "vgg_block_1_conv_2_weight",
//       "vgg_block_2_conv_1_bias", "vgg_block_2_conv_1_weight",
//       "vgg_block_2_conv_2_bias", "vgg_block_2_conv_2_weight",
//       "vgg_block_3_conv_1_bias", "vgg_block_3_conv_1_weight",
//       "vgg_block_3_conv_2_bias", "vgg_block_3_conv_2_weight",
//       "vgg_block_3_conv_3_bias", "vgg_block_3_conv_3_weight",
//       "vgg_block_4_conv_1_bias", "vgg_block_4_conv_1_weight",
//       "vgg_block_4_conv_2_bias", "vgg_block_4_conv_2_weight",
//       "vgg_block_4_conv_3_bias", "vgg_block_4_conv_3_weight"};

//   std::unique_ptr<model_spec> nn_spec = init_vgg_16(VGG_16_MODEL_PATH);
//   float_array_map weights = nn_spec->export_params_view();

//   for (const auto &weight : weights)
//     TS_ASSERT(expected_keys.count(weight.first) != 0);
// }

BOOST_AUTO_TEST_CASE(test_load_resnet) {
  /**
   * TODO: Delete this path once uploaded to https://developer.apple.com/
   **/
  std::string RESNET_MODEL_PATH =
      "/Users/abhishekpratapa/Desktop/mxnet_golden_set/testing_coreml_5_0.mlmodel";

  std::unique_ptr<model_spec> nn_spec = init_resnet(RESNET_MODEL_PATH, 8);
  float_array_map weights = nn_spec->export_params_view();

  for (const auto &weight : weights)
    std::cout << weight.first << std::endl;
}