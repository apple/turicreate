/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/style_transfer/style_transfer_model_definition.hpp>

#include <random>

#include <ml/neural_net/weight_init.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>

namespace turi {
namespace style_transfer {

using CoreML::Specification::InnerProductLayerParams;
using CoreML::Specification::NeuralNetwork;
using CoreML::Specification::NeuralNetworkLayer;

using turi::neural_net::float_array_map;
using turi::neural_net::model_spec;
using turi::neural_net::scalar_weight_initializer;
using turi::neural_net::uniform_weight_initializer;
using turi::neural_net::weight_initializer;
using turi::neural_net::zero_weight_initializer;


using padding_type = model_spec::padding_type;

namespace {

constexpr float LOWER_BOUND = -0.07;
constexpr float UPPER_BOUND = 0.07;

// TODO: refactor code to be more readable with loops
void define_resnet(model_spec& nn_spec, size_t num_styles, bool initialize=false, int random_seed=0) {
  std::mt19937 random_engine;
  std::seed_seq seed_seq{random_seed};
  random_engine = std::mt19937(seed_seq);

  weight_initializer initializer;

  // This is to make sure that when the uniform initialization is not needed extra work is avoided
  if (initialize) {
    initializer = uniform_weight_initializer(LOWER_BOUND, UPPER_BOUND, &random_engine);
  } else {
    initializer = zero_weight_initializer();
  }

  nn_spec.add_padding(
      /* name */ "transformer_pad0",
      /* input */ "image",
      /* padding_top */ 4,
      /* padding_bottom */ 4,
      /* padding_left */ 4,
      /* padding_right */ 4);

  nn_spec.add_convolution(
      /* name */ "transformer_encode_1_conv",
      /* input */ "transformer_pad0",
      /* num_output_channels */ 32,
      /* num_kernel_channels */ 3,
      /* kernel_height */ 9,
      /* kernel_width */ 9,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_encode_1_inst_gamma",
      /* input */ "index",
      /* num_output_channels */ 32,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_encode_1_inst_beta",
      /* input */ "index",
      /* num_output_channels */ 32,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm0__fwd_bn_",
      /* input */ "transformer_encode_1_conv",
      /* num_channels */ 32,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm0__fwd_bn_",
                    "transformer_encode_1_inst_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm0__fwd",
      /* inputs */ {"transformer_instancenorm0__fwd_mult_gamma",
                    "transformer_encode_1_inst_beta"});

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
      /* name */ "transformer_encode_2_conv",
      /* input */ "transformer_pad1",
      /* num_output_channels */ 64,
      /* num_kernel_channels */ 32,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 2,
      /* stride_width */ 2,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_encode_2_inst_gamma",
      /* input */ "index",
      /* num_output_channels */ 64,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_encode_2_inst_beta",
      /* input */ "index",
      /* num_output_channels */ 64,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm1__fwd_bn_",
      /* input */ "transformer_encode_2_conv",
      /* num_channels */ 64,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm1__fwd_bn_",
                    "transformer_encode_2_inst_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm1__fwd",
      /* inputs */ {"transformer_instancenorm1__fwd_mult_gamma",
                    "transformer_encode_2_inst_beta"});

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
      /* name */ "transformer_encode_3_conv",
      /* input */ "transformer_pad2",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 64,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 2,
      /* stride_width */ 2,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_encode_3_inst_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_encode_3_inst_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm2__fwd_bn_",
      /* input */ "transformer_encode_3_conv",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm2__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm2__fwd_bn_",
                    "transformer_encode_3_inst_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm2__fwd",
      /* inputs */ {"transformer_instancenorm2__fwd_mult_gamma",
                    "transformer_encode_3_inst_beta"});

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
      /* name */ "transformer_residual_1_conv_1",
      /* input */ "transformer_residualblock0_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_1_inst_1_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_1_inst_1_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock0_instancenorm0__fwd_bn_",
      /* input */ "transformer_residual_1_conv_1",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock0_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock0_instancenorm0__fwd_bn_",
                    "transformer_residual_1_inst_1_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock0_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock0_instancenorm0__fwd_mult_gamma",
                    "transformer_residual_1_inst_1_beta"});

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
      /* name */ "transformer_residual_1_conv_2",
      /* input */ "transformer_residualblock0_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_1_inst_2_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_1_inst_2_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock0_instancenorm1__fwd_bn_",
      /* input */ "transformer_residual_1_conv_2",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock0_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock0_instancenorm1__fwd_bn_",
                    "transformer_residual_1_inst_2_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock0_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock0_instancenorm1__fwd_mult_gamma",
                    "transformer_residual_1_inst_2_beta"});

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
      /* name */ "transformer_residual_2_conv_1",
      /* input */ "transformer_residualblock1_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_2_inst_1_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_2_inst_1_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock1_instancenorm0__fwd_bn_",
      /* input */ "transformer_residual_2_conv_1",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock1_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock1_instancenorm0__fwd_bn_",
                    "transformer_residual_2_inst_1_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock1_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock1_instancenorm0__fwd_mult_gamma",
                    "transformer_residual_2_inst_1_beta"});

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
      /* name */ "transformer_residual_2_conv_2",
      /* input */ "transformer_residualblock1_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_2_inst_2_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_2_inst_2_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock1_instancenorm1__fwd_bn_",
      /* input */ "transformer_residual_2_conv_2",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock1_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock1_instancenorm1__fwd_bn_",
                    "transformer_residual_2_inst_2_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock1_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock1_instancenorm1__fwd_mult_gamma",
                    "transformer_residual_2_inst_2_beta"});

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
      /* name */ "transformer_residual_3_conv_1",
      /* input */ "transformer_residualblock2_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_3_inst_1_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_3_inst_1_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock2_instancenorm0__fwd_bn_",
      /* input */ "transformer_residual_3_conv_1",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock2_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock2_instancenorm0__fwd_bn_",
                    "transformer_residual_3_inst_1_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock2_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock2_instancenorm0__fwd_mult_gamma",
                    "transformer_residual_3_inst_1_beta"});

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
      /* name */ "transformer_residual_3_conv_2",
      /* input */ "transformer_residualblock2_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_3_inst_2_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_3_inst_2_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock2_instancenorm1__fwd_bn_",
      /* input */ "transformer_residual_3_conv_2",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock2_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock2_instancenorm1__fwd_bn_",
                    "transformer_residual_3_inst_2_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock2_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock2_instancenorm1__fwd_mult_gamma",
                    "transformer_residual_3_inst_2_beta"});

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
      /* name */ "transformer_residual_4_conv_1",
      /* input */ "transformer_residualblock3_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_4_inst_1_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_4_inst_1_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock3_instancenorm0__fwd_bn_",
      /* input */ "transformer_residual_4_conv_1",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock3_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock3_instancenorm0__fwd_bn_",
                    "transformer_residual_4_inst_1_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock3_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock3_instancenorm0__fwd_mult_gamma",
                    "transformer_residual_4_inst_1_beta"});

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
      /* name */ "transformer_residual_4_conv_2",
      /* input */ "transformer_residualblock3_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_4_inst_2_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_4_inst_2_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock3_instancenorm1__fwd_bn_",
      /* input */ "transformer_residual_4_conv_2",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock3_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock3_instancenorm1__fwd_bn_",
                    "transformer_residual_4_inst_2_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock3_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock3_instancenorm1__fwd_mult_gamma",
                    "transformer_residual_4_inst_2_beta"});

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
      /* name */ "transformer_residual_5_conv_1",
      /* input */ "transformer_residualblock4_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_5_inst_1_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_5_inst_1_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock4_instancenorm0__fwd_bn_",
      /* input */ "transformer_residual_5_conv_1",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock4_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock4_instancenorm0__fwd_bn_",
                    "transformer_residual_5_inst_1_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock4_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock4_instancenorm0__fwd_mult_gamma",
                    "transformer_residual_5_inst_1_beta"});

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
      /* name */ "transformer_residual_5_conv_2",
      /* input */ "transformer_residualblock4_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_5_inst_2_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_residual_5_inst_2_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_residualblock4_instancenorm1__fwd_bn_",
      /* input */ "transformer_residual_5_conv_2",
      /* num_channels */ 128,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_residualblock4_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock4_instancenorm1__fwd_bn_",
                    "transformer_residual_5_inst_2_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_residualblock4_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock4_instancenorm1__fwd_mult_gamma",
                    "transformer_residual_5_inst_2_beta"});

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
      /* name */ "transformer_decoding_1_conv",
      /* input */ "transformer_pad3",
      /* num_output_channels */ 64,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_decoding_1_inst_gamma",
      /* input */ "index",
      /* num_output_channels */ 64,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_decoding_1_inst_beta",
      /* input */ "index",
      /* num_output_channels */ 64,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm3__fwd_bn_",
      /* input */ "transformer_decoding_1_conv",
      /* num_channels */ 64,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm3__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm3__fwd_bn_",
                    "transformer_decoding_1_inst_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm3__fwd",
      /* inputs */ {"transformer_instancenorm3__fwd_mult_gamma",
                    "transformer_decoding_1_inst_beta"});

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
      /* name */ "transformer_decoding_2_conv",
      /* input */ "transformer_pad4",
      /* num_output_channels */ 32,
      /* num_kernel_channels */ 64,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_decoding_2_inst_gamma",
      /* input */ "index",
      /* num_output_channels */ 32,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_decoding_2_inst_beta",
      /* input */ "index",
      /* num_output_channels */ 32,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm4__fwd_bn_",
      /* input */ "transformer_decoding_2_conv",
      /* num_channels */ 32,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm4__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm4__fwd_bn_",
                    "transformer_decoding_2_inst_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm4__fwd",
      /* inputs */ {"transformer_instancenorm4__fwd_mult_gamma",
                    "transformer_decoding_2_inst_beta"});

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
      /* name */ "transformer_conv5",
      /* input */ "transformer_pad5",
      /* num_output_channels */ 3,
      /* num_kernel_channels */ 32,
      /* kernel_height */ 9,
      /* kernel_width */ 9,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ initializer);

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm5_gamma",
      /* input */ "index",
      /* num_output_channels */ 3,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_inner_product(
      /* name */ "transformer_instancenorm5_beta",
      /* input */ "index",
      /* num_output_channels */ 3,
      /* num_input_channels */ num_styles,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec.add_instancenorm(
      /* name */ "transformer_instancenorm5__fwd_bn_",
      /* input */ "transformer_conv5",
      /* num_channels */ 3,
      /* epsilon */ 1e-5);

  nn_spec.add_multiplication(
      /* name */ "transformer_instancenorm5__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm5__fwd_bn_",
                    "transformer_instancenorm5_gamma"});

  nn_spec.add_addition(
      /* name */ "transformer_instancenorm5__fwd",
      /* inputs */ {"transformer_instancenorm5__fwd_mult_gamma",
                    "transformer_instancenorm5_beta"});

  nn_spec.add_sigmoid(
      /* name */ "transformer_activation5",
      /* input */ "transformer_instancenorm5__fwd");

  nn_spec.add_scale(
      /* name */ "stylizedImage",
      /* input */ "transformer_activation5",
      /* shape_c */ {1},
      /* weight_init_fn */ scalar_weight_initializer(255.0));
}

// TODO: refactor code to be more readable with loops
void define_vgg(model_spec& nn_spec) {
  nn_spec.add_convolution(
      /* name */ "vgg_block_1_conv_1",
      /* input */ "image",
      /* num_output_channels */ 64,
      /* num_kernel_channels */ 3,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::SAME,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_relu(
      /* name */ "vgg16_activation0",
      /* input */ "vgg_block_1_conv_1");

  nn_spec.add_convolution(
      /* name */ "vgg_block_1_conv_2",
      /* input */ "vgg16_activation0",
      /* num_output_channels */ 64,
      /* num_kernel_channels */ 64,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::SAME,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_relu(
      /* name */ "vgg16_activation1",
      /* input */ "vgg_block_1_conv_2");

  nn_spec.add_pooling(
      /* name */ "vgg16_pooling0",
      /* input */ "vgg16_activation1",
      /* kernel_height */ 2,
      /* kernel_width */ 2,
      /* stride_h */ 2,
      /* stride_w */ 2,
      /* padding */ padding_type::VALID,
      /* use_poolexcludepadding */ false);

  nn_spec.add_convolution(
      /* name */ "vgg_block_2_conv_1",
      /* input */ "vgg16_pooling0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 64,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::SAME,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_relu(
      /* name */ "vgg16_activation2",
      /* input */ "vgg_block_2_conv_1");

  nn_spec.add_convolution(
      /* name */ "vgg_block_2_conv_2",
      /* input */ "vgg16_activation2",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::SAME,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_relu(
      /* name */ "vgg16_activation3",
      /* input */ "vgg_block_2_conv_2");

  nn_spec.add_pooling(
      /* name */ "vgg16_pooling1",
      /* input */ "vgg16_activation3",
      /* kernel_height */ 2,
      /* kernel_width */ 2,
      /* stride_h */ 2,
      /* stride_w */ 2,
      /* padding */ padding_type::VALID,
      /* use_poolexcludepadding */ false);

  nn_spec.add_convolution(
      /* name */ "vgg_block_3_conv_1",
      /* input */ "vgg16_pooling1",
      /* num_output_channels */ 256,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::SAME,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_relu(
      /* name */ "vgg16_activation4",
      /* input */ "vgg_block_3_conv_1");

  nn_spec.add_convolution(
      /* name */ "vgg_block_3_conv_2",
      /* input */ "vgg16_activation4",
      /* num_output_channels */ 256,
      /* num_kernel_channels */ 256,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::SAME,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_relu(
      /* name */ "vgg16_activation5",
      /* input */ "vgg_block_3_conv_2");

  nn_spec.add_convolution(
      /* name */ "vgg_block_3_conv_3",
      /* input */ "vgg16_activation5",
      /* num_output_channels */ 256,
      /* num_kernel_channels */ 256,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::SAME,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_relu(
      /* name */ "vgg16_activation6",
      /* input */ "vgg_block_3_conv_3");

  nn_spec.add_pooling(
      /* name */ "vgg16_pooling2",
      /* input */ "vgg16_activation6",
      /* kernel_height */ 2,
      /* kernel_width */ 2,
      /* stride_h */ 2,
      /* stride_w */ 2,
      /* padding */ padding_type::VALID,
      /* use_poolexcludepadding */ false);

  nn_spec.add_convolution(
      /* name */ "vgg_block_4_conv_1",
      /* input */ "vgg16_pooling2",
      /* num_output_channels */ 512,
      /* num_kernel_channels */ 256,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::SAME,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_relu(
      /* name */ "vgg16_activation7",
      /* input */ "vgg_block_4_conv_1");

  nn_spec.add_convolution(
      /* name */ "vgg_block_4_conv_2",
      /* input */ "vgg16_activation7",
      /* num_output_channels */ 512,
      /* num_kernel_channels */ 512,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::SAME,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_relu(
      /* name */ "vgg16_activation8",
      /* input */ "vgg_block_4_conv_2");

  nn_spec.add_convolution(
      /* name */ "vgg_block_4_conv_3",
      /* input */ "vgg16_activation8",
      /* num_output_channels */ 512,
      /* num_kernel_channels */ 512,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::SAME,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec.add_relu(
      /* name */ "vgg16_activation9_output",
      /* input */ "vgg_block_4_conv_3");
}

void load_weights(model_spec& nn_spec, const std::string& path) {
  model_spec weight_spec(path);
  float_array_map nn_params = weight_spec.export_params_view();

  auto map_iter = nn_params.begin();
  while (map_iter != nn_params.end()) {
    if (map_iter->first.find("inst") != std::string::npos) {
      nn_params.erase(map_iter++);
    } else {
      ++map_iter;
    }
  }

  nn_spec.update_params(nn_params);
}

}  // namespace

std::unique_ptr<model_spec> init_resnet(const std::string& path) {
  std::unique_ptr<model_spec> spec(new model_spec(path));
  return spec;
}

std::unique_ptr<neural_net::model_spec> init_resnet(size_t num_styles,
                                                    int random_seed) {
  std::unique_ptr<model_spec> nn_spec(new model_spec());
  define_resnet(*nn_spec, num_styles, /* initialize */ true, random_seed);
  return nn_spec;
}

std::unique_ptr<neural_net::model_spec> init_resnet(const std::string& path,
                                                    size_t num_styles) {
  std::unique_ptr<model_spec> nn_spec(new model_spec());
  define_resnet(*nn_spec, num_styles);
  load_weights(*nn_spec, path);
  return nn_spec;
}

std::unique_ptr<model_spec> init_vgg_16() {
  std::unique_ptr<model_spec> nn_spec(new model_spec());
  define_vgg(*nn_spec);
  return nn_spec;
}

std::unique_ptr<model_spec> init_vgg_16(const std::string& path) {
  std::unique_ptr<model_spec> nn_spec(new model_spec(path));
  return nn_spec;
}

}  // namespace style_transfer
}  // namespace turi
