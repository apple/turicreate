/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/style_transfer/style_transfer_model_definition.hpp>

#include <ml/neural_net/weight_init.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>

namespace turi {
namespace style_transfer {

using CoreML::Specification::InnerProductLayerParams;
using CoreML::Specification::NeuralNetwork;
using CoreML::Specification::NeuralNetworkLayer;

using turi::neural_net::model_spec;
using turi::neural_net::ones_weight_initializer;
using turi::neural_net::zero_weight_initializer;

namespace {

std::unique_ptr<neural_net::model_spec> update_num_styles(
    std::unique_ptr<model_spec>& spec, size_t num_styles) {
  CoreML::Specification::NeuralNetwork neural_net = spec->get_coreml_spec();

  for (NeuralNetworkLayer& layer : *neural_net.mutable_layers()) {
    if (layer.name().find("_inst_") != std::string::npos) {
      InnerProductLayerParams* params = layer.mutable_innerproduct();
      params->set_inputchannels(num_styles);

      size_t weights_size = params->inputchannels() * params->outputchannels();

      if (layer.name().find("gamma") != std::string::npos) {
        init_weight_params(params->mutable_weights(), weights_size,
                           ones_weight_initializer());
      } else {
        init_weight_params(params->mutable_weights(), weights_size,
                           zero_weight_initializer());
      }
    }
  }

  return std::unique_ptr<model_spec>(new model_spec(neural_net));
}

}  // namespace

std::unique_ptr<model_spec> init_resnet(std::string& path) {
  std::unique_ptr<model_spec> spec(new model_spec(path));
  return spec;
}

std::unique_ptr<neural_net::model_spec> init_resnet(size_t num_styles) {
  std::unique_ptr<model_spec> nn_spec(new model_spec());

  nn_spec.add_padding(
      /* name */ "transformer_pad0",
      /* input */ "image",
      /* padding_top */ 4,
      /* padding_bottom */ 4,
      /* padding_left */ 4,
      /* padding_right */ 4);

  nn_spec.add_convolution(
      /* name */ "transformer_conv0_fwd",
      /* input */ "transformer_pad0",
      /* num_output_channels */ 32,
      /* num_kernel_channels */ 3,
      /* kernel_height */ 9,
      /* kernel_width */ 9,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm0_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 32,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm0_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 32,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm0__fwd_bn_",
      /* input */
      "transformer_conv0_fwd"
      /* num_channels */ 32,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm0__fwd_bn_",
                    "transformer_instancenorm0_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm0__fwd",
      /* inputs */ {"transformer_instancenorm0__fwd_mult_gamma",
                    "transformer_instancenorm0_embedding1"});

  nn_spec.add_relu(
      /* name */ "transformer_activation0",
      /* input */ "transformer_instancenorm0__fwd");

  nn_spec.add_padding(
      /* name */ "transformer_pad1",
      /* input */ "transformer_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_conv1_fwd",
      /* input */ "transformer_pad1",
      /* num_output_channels */ 64,
      /* num_kernel_channels */ 32,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 2,
      /* stride_width */ 2,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm1_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 64,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm1_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 64,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm1__fwd_bn_",
      /* input */
      "transformer_conv1_fwd"
      /* num_channels */ 64,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm1__fwd_bn_",
                    "transformer_instancenorm1_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm1__fwd",
      /* inputs */ {"transformer_instancenorm1__fwd_mult_gamma",
                    "transformer_instancenorm1_embedding1"});

  nn_spec.add_relu(
      /* name */ "transformer_activation1",
      /* input */ "transformer_instancenorm1__fwd");

  nn_spec.add_padding(
      /* name */ "transformer_pad2",
      /* input */ "transformer_activation1",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_conv2_fwd",
      /* input */ "transformer_pad2",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 64,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 2,
      /* stride_width */ 2,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm2_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm2_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm2__fwd_bn_",
      /* input */
      "transformer_conv2_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm2__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm2__fwd_bn_",
                    "transformer_instancenorm2_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm2__fwd",
      /* inputs */ {"transformer_instancenorm2__fwd_mult_gamma",
                    "transformer_instancenorm2_embedding1"});

  nn_spec.add_relu(
      /* name */ "transformer_activation2",
      /* input */ "transformer_instancenorm2__fwd");

  nn_spec.add_padding(
      /* name */ "transformer_residualblock0_pad0",
      /* input */ "transformer_activation2",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_residualblock0_conv0_fwd",
      /* input */ "transformer_residualblock0_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock0_instancenorm0_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock0_instancenorm0_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock0_instancenorm0__fwd_bn_",
      /* input */
      "transformer_residualblock0_conv0_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock0_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock0_instancenorm0__fwd_bn_",
                    "transformer_residualblock0_instancenorm0_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock0_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock0_instancenorm0__fwd_mult_gamma",
                    "transformer_residualblock0_instancenorm0_embedding1"});

  nn_spec.add_relu(
      /* name */ "transformer_residualblock0_activation0",
      /* input */ "transformer_residualblock0_instancenorm0__fwd");

  nn_spec.add_padding(
      /* name */ "transformer_residualblock0_pad1",
      /* input */ "transformer_residualblock0_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_residualblock0_conv1_fwd",
      /* input */ "transformer_residualblock0_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock0_instancenorm1_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock0_instancenorm1_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock0_instancenorm1__fwd_bn_",
      /* input */
      "transformer_residualblock0_conv1_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock0_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock0_instancenorm1__fwd_bn_",
                    "transformer_residualblock0_instancenorm1_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock0_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock0_instancenorm1__fwd_mult_gamma",
                    "transformer_residualblock0_instancenorm1_embedding1"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock0__plus0",
      /* inputs */ {"transformer_activation2",
                    "transformer_residualblock0_instancenorm1__fwd"});

  nn_spec.add_padding(
      /* name */ "transformer_residualblock1_pad0",
      /* input */ "transformer_residualblock0__plus0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_residualblock1_conv0_fwd",
      /* input */ "transformer_residualblock1_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock1_instancenorm0_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock1_instancenorm0_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock1_instancenorm0__fwd_bn_",
      /* input */
      "transformer_residualblock1_conv0_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock1_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock1_instancenorm0__fwd_bn_",
                    "transformer_residualblock1_instancenorm0_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock1_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock1_instancenorm0__fwd_mult_gamma",
                    "transformer_residualblock1_instancenorm0_embedding1"});

  nn_spec.add_relu(
      /* name */ "transformer_residualblock1_activation0",
      /* input */ "transformer_residualblock1_instancenorm0__fwd");

  nn_spec.add_padding(
      /* name */ "transformer_residualblock1_pad1",
      /* input */ "transformer_residualblock1_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_residualblock1_conv1_fwd",
      /* input */ "transformer_residualblock1_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock1_instancenorm1_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock1_instancenorm1_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock1_instancenorm1__fwd_bn_",
      /* input */
      "transformer_residualblock1_conv1_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock1_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock1_instancenorm1__fwd_bn_",
                    "transformer_residualblock1_instancenorm1_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock1_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock1_instancenorm1__fwd_mult_gamma",
                    "transformer_residualblock1_instancenorm1_embedding1"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock1__plus0",
      /* inputs */ {"transformer_residualblock0__plus0",
                    "transformer_residualblock1_instancenorm1__fwd"});

  nn_spec.add_padding(
      /* name */ "transformer_residualblock2_pad0",
      /* input */ "transformer_residualblock1__plus0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_residualblock2_conv0_fwd",
      /* input */ "transformer_residualblock2_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock2_instancenorm0_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock2_instancenorm0_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock2_instancenorm0__fwd_bn_",
      /* input */
      "transformer_residualblock2_conv0_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock2_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock2_instancenorm0__fwd_bn_",
                    "transformer_residualblock2_instancenorm0_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock2_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock2_instancenorm0__fwd_mult_gamma",
                    "transformer_residualblock2_instancenorm0_embedding1"});

  nn_spec.add_relu(
      /* name */ "transformer_residualblock2_activation0",
      /* input */ "transformer_residualblock2_instancenorm0__fwd");

  nn_spec.add_padding(
      /* name */ "transformer_residualblock2_pad1",
      /* input */ "transformer_residualblock2_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_residualblock2_conv1_fwd",
      /* input */ "transformer_residualblock2_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock2_instancenorm1_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock2_instancenorm1_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock2_instancenorm1__fwd_bn_",
      /* input */
      "transformer_residualblock2_conv1_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock2_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock2_instancenorm1__fwd_bn_",
                    "transformer_residualblock2_instancenorm1_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock2_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock2_instancenorm1__fwd_mult_gamma",
                    "transformer_residualblock2_instancenorm1_embedding1"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock2__plus0",
      /* inputs */ {"transformer_residualblock1__plus0",
                    "transformer_residualblock2_instancenorm1__fwd"});

  nn_spec.add_padding(
      /* name */ "transformer_residualblock3_pad0",
      /* input */ "transformer_residualblock2__plus0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_residualblock3_conv0_fwd",
      /* input */ "transformer_residualblock3_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock3_instancenorm0_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock3_instancenorm0_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock3_instancenorm0__fwd_bn_",
      /* input */
      "transformer_residualblock3_conv0_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock3_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock3_instancenorm0__fwd_bn_",
                    "transformer_residualblock3_instancenorm0_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock3_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock3_instancenorm0__fwd_mult_gamma",
                    "transformer_residualblock3_instancenorm0_embedding1"});

  nn_spec.add_relu(
      /* name */ "transformer_residualblock3_activation0",
      /* input */ "transformer_residualblock3_instancenorm0__fwd");

  nn_spec.add_padding(
      /* name */ "transformer_residualblock3_pad1",
      /* input */ "transformer_residualblock3_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_residualblock3_conv1_fwd",
      /* input */ "transformer_residualblock3_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock3_instancenorm1_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock3_instancenorm1_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock3_instancenorm1__fwd_bn_",
      /* input */
      "transformer_residualblock3_conv1_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock3_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock3_instancenorm1__fwd_bn_",
                    "transformer_residualblock3_instancenorm1_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock3_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock3_instancenorm1__fwd_mult_gamma",
                    "transformer_residualblock3_instancenorm1_embedding1"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock3__plus0",
      /* inputs */ {"transformer_residualblock2__plus0",
                    "transformer_residualblock3_instancenorm1__fwd"});

  nn_spec.add_padding(
      /* name */ "transformer_residualblock4_pad0",
      /* input */ "transformer_residualblock3__plus0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_residualblock4_conv0_fwd",
      /* input */ "transformer_residualblock4_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock4_instancenorm0_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock4_instancenorm0_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock4_instancenorm0__fwd_bn_",
      /* input */
      "transformer_residualblock4_conv0_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock4_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock4_instancenorm0__fwd_bn_",
                    "transformer_residualblock4_instancenorm0_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock4_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock4_instancenorm0__fwd_mult_gamma",
                    "transformer_residualblock4_instancenorm0_embedding1"});

  nn_spec.add_relu(
      /* name */ "transformer_residualblock4_activation0",
      /* input */ "transformer_residualblock4_instancenorm0__fwd");

  nn_spec.add_padding(
      /* name */ "transformer_residualblock4_pad1",
      /* input */ "transformer_residualblock4_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_residualblock4_conv1_fwd",
      /* input */ "transformer_residualblock4_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock4_instancenorm1_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residualblock4_instancenorm1_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 128,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock4_instancenorm1__fwd_bn_",
      /* input */
      "transformer_residualblock4_conv1_fwd"
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock4_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock4_instancenorm1__fwd_bn_",
                    "transformer_residualblock4_instancenorm1_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock4_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock4_instancenorm1__fwd_mult_gamma",
                    "transformer_residualblock4_instancenorm1_embedding1"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock4__plus0",
      /* inputs */ {"transformer_residualblock3__plus0",
                    "transformer_residualblock4_instancenorm1__fwd"});

  nn_spec.add_upsampling(
      /* name */ "transformer_upsampling0",
      /* input */ "transformer_residualblock4__plus0",
      /* scaling_x */ 2,
      /* scaling_y */ 2);

  nn_spec.add_padding(
      /* name */ "transformer_pad3",
      /* input */ "transformer_upsampling0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_conv3_fwd",
      /* input */ "transformer_pad3",
      /* num_output_channels */ 64,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm3_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 64,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm3_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 64,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm3__fwd_bn_",
      /* input */
      "transformer_conv3_fwd"
      /* num_channels */ 64,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm3__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm3__fwd_bn_",
                    "transformer_instancenorm3_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm3__fwd",
      /* inputs */ {"transformer_instancenorm3__fwd_mult_gamma",
                    "transformer_instancenorm3_embedding1"});

  nn_spec.add_relu(
      /* name */ "transformer_activation3",
      /* input */ "transformer_instancenorm3__fwd");

  nn_spec.add_upsampling(
      /* name */ "transformer_upsampling1",
      /* input */ "transformer_activation3",
      /* scaling_x */ 2,
      /* scaling_y */ 2);

  nn_spec.add_padding(
      /* name */ "transformer_pad4",
      /* input */ "transformer_upsampling1",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec.add_convolution(
      /* name */ "transformer_conv4_fwd",
      /* input */ "transformer_pad4",
      /* num_output_channels */ 32,
      /* num_kernel_channels */ 64,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm4_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 32,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm4_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 32,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm4__fwd_bn_",
      /* input */
      "transformer_conv4_fwd"
      /* num_channels */ 32,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm4__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm4__fwd_bn_",
                    "transformer_instancenorm4_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm4__fwd",
      /* inputs */ {"transformer_instancenorm4__fwd_mult_gamma",
                    "transformer_instancenorm4_embedding1"});

  nn_spec.add_relu(
      /* name */ "transformer_activation4",
      /* input */ "transformer_instancenorm4__fwd");

  nn_spec.add_padding(
      /* name */ "transformer_pad5",
      /* input */ "transformer_activation4",
      /* padding_top */ 4,
      /* padding_bottom */ 4,
      /* padding_left */ 4,
      /* padding_right */ 4);

  nn_spec.add_convolution(
      /* name */ "transformer_conv5_fwd",
      /* input */ "transformer_pad5",
      /* num_output_channels */ 3,
      /* num_kernel_channels */ 32,
      /* kernel_height */ 9,
      /* kernel_width */ 9,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm5_embedding0",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 3,
      /* weight_init_fn */ ones_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm5_embedding1",
      /* input */ "index",
      /* num_output_channels */ 1,
      /* num_input_channels */ 3,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm5__fwd_bn_",
      /* input */
      "transformer_conv5_fwd"
      /* num_channels */ 3,
      /* epsilon */ 9.99999974738e-06);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm5__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm5__fwd_bn_",
                    "transformer_instancenorm5_embedding0"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm5__fwd",
      /* inputs */ {"transformer_instancenorm5__fwd_mult_gamma",
                    "transformer_instancenorm5_embedding1"});

  nn_spec.add_sigmoid(
      /* name */ "transformer_activation5",
      /* input */ "transformer_instancenorm5__fwd");

  nn_spec.add_scale(
      /* name */ "stylizedImage",
      /* input */ "transformer_activation5",
      /* shape_c_h_w */ {1},
      /* weight_init_fn */ const_weight_initializer(255.0));

  return nn_spec;
}

std::unique_ptr<model_spec> init_vgg_16(std::string& path) {
  std::unique_ptr<model_spec> spec(new model_spec(path));
  return spec;
}

}  // namespace style_transfer
}  // namespace turi