/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include "utils.hpp"

#include <ml/neural_net/weight_init.hpp>
#include <model_server/lib/image_util.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <toolkits/coreml_export/neural_net_models_exporter.hpp>

using CoreML::Specification::ArrayFeatureType;
using CoreML::Specification::FeatureDescription;
using CoreML::Specification::ImageFeatureType;
using CoreML::Specification::ModelDescription;
using CoreML::Specification::NeuralNetworkLayer;
using turi::coreml::MLModelWrapper;
using turi::neural_net::model_spec;
using padding_type = turi::neural_net::model_spec::padding_type;
using turi::neural_net::scalar_weight_initializer;
using turi::neural_net::zero_weight_initializer;

namespace turi {
namespace style_transfer {

std::string generate_data(size_t data_size) {
  std::string data_array;
  data_array.reserve(data_size);
  for (size_t x = 0; x < data_size; x++) data_array[x] = static_cast<char>((uint8_t)(rand() % 256));

  return data_array;
}

turi::flex_image random_image() {
  size_t height = ((size_t)(rand() % 10) + 15);
  size_t width = ((size_t)(rand() % 10) + 15);
  size_t channels = 3;

  size_t data_size = height * width * channels;

  size_t image_type_version = IMAGE_TYPE_CURRENT_VERSION;
  size_t format = 2;

  std::string img_data = generate_data(data_size);

  return turi::image_type(img_data.c_str(), height, width, channels, data_size, image_type_version,
                          format);
}

turi::gl_sarray random_image_sarray(size_t length) {
  std::vector<turi::flexible_type> image_column_data;
  for (size_t x = 0; x < length; x++) image_column_data.push_back(random_image());

  turi::gl_sarray sa;
  sa.construct_from_vector(image_column_data, turi::flex_type_enum::IMAGE);

  return sa;
}

turi::gl_sframe random_sframe(size_t length, std::string image_column_name) {
  turi::gl_sarray image_sa = random_image_sarray(length);
  turi::gl_sframe image_sf;
  image_sf.add_column(image_sa, image_column_name);

  return image_sf;
}

std::string get_vgg16_model() {
  std::unique_ptr<model_spec> nn_spec(new model_spec());

  nn_spec->add_convolution(
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

  nn_spec->add_relu(
      /* name */ "vgg16_activation0",
      /* input */ "vgg_block_1_conv_1");

  nn_spec->add_convolution(
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

  nn_spec->add_relu(
      /* name */ "vgg16_activation1",
      /* input */ "vgg_block_1_conv_2");

  nn_spec->add_pooling(
      /* name */ "vgg16_pooling0",
      /* input */ "vgg16_activation1",
      /* kernel_height */ 2,
      /* kernel_width */ 2,
      /* stride_h */ 2,
      /* stride_w */ 2,
      /* padding */ padding_type::VALID,
      /* use_poolexcludepadding */ false);

  nn_spec->add_convolution(
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

  nn_spec->add_relu(
      /* name */ "vgg16_activation2",
      /* input */ "vgg_block_2_conv_1");

  nn_spec->add_convolution(
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

  nn_spec->add_relu(
      /* name */ "vgg16_activation3",
      /* input */ "vgg_block_2_conv_2");

  nn_spec->add_pooling(
      /* name */ "vgg16_pooling1",
      /* input */ "vgg16_activation3",
      /* kernel_height */ 2,
      /* kernel_width */ 2,
      /* stride_h */ 2,
      /* stride_w */ 2,
      /* padding */ padding_type::VALID,
      /* use_poolexcludepadding */ false);

  nn_spec->add_convolution(
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

  nn_spec->add_relu(
      /* name */ "vgg16_activation4",
      /* input */ "vgg_block_3_conv_1");

  nn_spec->add_convolution(
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

  nn_spec->add_relu(
      /* name */ "vgg16_activation5",
      /* input */ "vgg_block_3_conv_2");

  nn_spec->add_convolution(
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

  nn_spec->add_relu(
      /* name */ "vgg16_activation6",
      /* input */ "vgg_block_3_conv_3");

  nn_spec->add_pooling(
      /* name */ "vgg16_pooling2",
      /* input */ "vgg16_activation6",
      /* kernel_height */ 2,
      /* kernel_width */ 2,
      /* stride_h */ 2,
      /* stride_w */ 2,
      /* padding */ padding_type::VALID,
      /* use_poolexcludepadding */ false);

  nn_spec->add_convolution(
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

  nn_spec->add_relu(
      /* name */ "vgg16_activation7",
      /* input */ "vgg_block_4_conv_1");

  nn_spec->add_convolution(
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

  nn_spec->add_relu(
      /* name */ "vgg16_activation8",
      /* input */ "vgg_block_4_conv_2");

  nn_spec->add_convolution(
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

  nn_spec->add_relu(
      /* name */ "vgg16_activation9_output",
      /* input */ "vgg_block_4_conv_3");

  const std::string VGG16_NAME = "./vgg16.mlmodel";

  CoreML::Specification::Model model;
  model.set_specificationversion(1);

  // Set Input
  ModelDescription* model_desc = model.mutable_description();

  FeatureDescription* input_feature_desc = model_desc->add_input();
  input_feature_desc->set_name("image");

  ImageFeatureType* input_image_feature = input_feature_desc->mutable_type()->mutable_imagetype();
  input_image_feature->set_width(256);
  input_image_feature->set_height(256);
  input_image_feature->set_colorspace(ImageFeatureType::RGB);

  // Set Output
  FeatureDescription* output_feature_desc = model_desc->add_output();
  output_feature_desc->set_name("vgg16_activation9_output");

  ImageFeatureType* output_image_feature = output_feature_desc->mutable_type()->mutable_imagetype();
  output_image_feature->set_width(256);
  output_image_feature->set_height(256);
  output_image_feature->set_colorspace(ImageFeatureType::RGB);

  model.mutable_neuralnetwork()->MergeFrom(nn_spec->get_coreml_spec());

  auto model_wrapper = std::make_shared<MLModelWrapper>(std::make_shared<CoreML::Model>(model));

  model_wrapper->save(VGG16_NAME);

  return VGG16_NAME;
}

std::string get_resnet_model() {
  std::unique_ptr<model_spec> nn_spec(new model_spec());

  nn_spec->add_padding(
      /* name */ "transformer_pad0",
      /* input */ "image",
      /* padding_top */ 4,
      /* padding_bottom */ 4,
      /* padding_left */ 4,
      /* padding_right */ 4);

  nn_spec->add_convolution(
      /* name */ "transformer_encode_1_conv",
      /* input */ "transformer_pad0",
      /* num_output_channels */ 32,
      /* num_kernel_channels */ 3,
      /* kernel_height */ 9,
      /* kernel_width */ 9,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_encode_1_inst_gamma",
      /* input */ "index",
      /* num_output_channels */ 32,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_encode_1_inst_beta",
      /* input */ "index",
      /* num_output_channels */ 32,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_instancenorm0__fwd_bn_",
      /* input */ "transformer_encode_1_conv",
      /* num_channels */ 32,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm0__fwd_bn_", "transformer_encode_1_inst_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_instancenorm0__fwd",
      /* inputs */ {"transformer_instancenorm0__fwd_mult_gamma", "transformer_encode_1_inst_beta"});

  nn_spec->add_relu(
      /* name */ "transformer_activation0",
      /* input */ "transformer_instancenorm0__fwd");

  nn_spec->add_padding(
      /* name */ "transformer_pad1",
      /* input */ "transformer_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_encode_2_conv",
      /* input */ "transformer_pad1",
      /* num_output_channels */ 64,
      /* num_kernel_channels */ 32,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 2,
      /* stride_width */ 2,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_encode_2_inst_gamma",
      /* input */ "index",
      /* num_output_channels */ 64,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_encode_2_inst_beta",
      /* input */ "index",
      /* num_output_channels */ 64,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_instancenorm1__fwd_bn_",
      /* input */ "transformer_encode_2_conv",
      /* num_channels */ 64,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm1__fwd_bn_", "transformer_encode_2_inst_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_instancenorm1__fwd",
      /* inputs */ {"transformer_instancenorm1__fwd_mult_gamma", "transformer_encode_2_inst_beta"});

  nn_spec->add_relu(
      /* name */ "transformer_activation1",
      /* input */ "transformer_instancenorm1__fwd");

  nn_spec->add_padding(
      /* name */ "transformer_pad2",
      /* input */ "transformer_activation1",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_encode_3_conv",
      /* input */ "transformer_pad2",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 64,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 2,
      /* stride_width */ 2,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_encode_3_inst_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_encode_3_inst_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_instancenorm2__fwd_bn_",
      /* input */ "transformer_encode_3_conv",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_instancenorm2__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm2__fwd_bn_", "transformer_encode_3_inst_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_instancenorm2__fwd",
      /* inputs */ {"transformer_instancenorm2__fwd_mult_gamma", "transformer_encode_3_inst_beta"});

  nn_spec->add_relu(
      /* name */ "transformer_activation2",
      /* input */ "transformer_instancenorm2__fwd");

  nn_spec->add_padding(
      /* name */ "transformer_residualblock0_pad0",
      /* input */ "transformer_activation2",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_residual_1_conv_1",
      /* input */ "transformer_residualblock0_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_1_inst_1_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_1_inst_1_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_residualblock0_instancenorm0__fwd_bn_",
      /* input */ "transformer_residual_1_conv_1",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_residualblock0_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock0_instancenorm0__fwd_bn_",
                    "transformer_residual_1_inst_1_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock0_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock0_instancenorm0__fwd_mult_gamma",
                    "transformer_residual_1_inst_1_beta"});

  nn_spec->add_relu(
      /* name */ "transformer_residualblock0_activation0",
      /* input */ "transformer_residualblock0_instancenorm0__fwd");

  nn_spec->add_padding(
      /* name */ "transformer_residualblock0_pad1",
      /* input */ "transformer_residualblock0_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_residual_1_conv_2",
      /* input */ "transformer_residualblock0_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_1_inst_2_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_1_inst_2_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_residualblock0_instancenorm1__fwd_bn_",
      /* input */ "transformer_residual_1_conv_2",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_residualblock0_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock0_instancenorm1__fwd_bn_",
                    "transformer_residual_1_inst_2_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock0_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock0_instancenorm1__fwd_mult_gamma",
                    "transformer_residual_1_inst_2_beta"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock0__plus0",
      /* inputs */ {"transformer_activation2", "transformer_residualblock0_instancenorm1__fwd"});

  nn_spec->add_padding(
      /* name */ "transformer_residualblock1_pad0",
      /* input */ "transformer_residualblock0__plus0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_residual_2_conv_1",
      /* input */ "transformer_residualblock1_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_2_inst_1_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_2_inst_1_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_residualblock1_instancenorm0__fwd_bn_",
      /* input */ "transformer_residual_2_conv_1",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_residualblock1_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock1_instancenorm0__fwd_bn_",
                    "transformer_residual_2_inst_1_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock1_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock1_instancenorm0__fwd_mult_gamma",
                    "transformer_residual_2_inst_1_beta"});

  nn_spec->add_relu(
      /* name */ "transformer_residualblock1_activation0",
      /* input */ "transformer_residualblock1_instancenorm0__fwd");

  nn_spec->add_padding(
      /* name */ "transformer_residualblock1_pad1",
      /* input */ "transformer_residualblock1_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_residual_2_conv_2",
      /* input */ "transformer_residualblock1_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_2_inst_2_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_2_inst_2_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_residualblock1_instancenorm1__fwd_bn_",
      /* input */ "transformer_residual_2_conv_2",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_residualblock1_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock1_instancenorm1__fwd_bn_",
                    "transformer_residual_2_inst_2_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock1_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock1_instancenorm1__fwd_mult_gamma",
                    "transformer_residual_2_inst_2_beta"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock1__plus0",
      /* inputs */ {"transformer_residualblock0__plus0",
                    "transformer_residualblock1_instancenorm1__fwd"});

  nn_spec->add_padding(
      /* name */ "transformer_residualblock2_pad0",
      /* input */ "transformer_residualblock1__plus0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_residual_3_conv_1",
      /* input */ "transformer_residualblock2_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_3_inst_1_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_3_inst_1_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_residualblock2_instancenorm0__fwd_bn_",
      /* input */ "transformer_residual_3_conv_1",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_residualblock2_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock2_instancenorm0__fwd_bn_",
                    "transformer_residual_3_inst_1_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock2_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock2_instancenorm0__fwd_mult_gamma",
                    "transformer_residual_3_inst_1_beta"});

  nn_spec->add_relu(
      /* name */ "transformer_residualblock2_activation0",
      /* input */ "transformer_residualblock2_instancenorm0__fwd");

  nn_spec->add_padding(
      /* name */ "transformer_residualblock2_pad1",
      /* input */ "transformer_residualblock2_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_residual_3_conv_2",
      /* input */ "transformer_residualblock2_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_3_inst_2_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_3_inst_2_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_residualblock2_instancenorm1__fwd_bn_",
      /* input */ "transformer_residual_3_conv_2",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_residualblock2_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock2_instancenorm1__fwd_bn_",
                    "transformer_residual_3_inst_2_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock2_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock2_instancenorm1__fwd_mult_gamma",
                    "transformer_residual_3_inst_2_beta"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock2__plus0",
      /* inputs */ {"transformer_residualblock1__plus0",
                    "transformer_residualblock2_instancenorm1__fwd"});

  nn_spec->add_padding(
      /* name */ "transformer_residualblock3_pad0",
      /* input */ "transformer_residualblock2__plus0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_residual_4_conv_1",
      /* input */ "transformer_residualblock3_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_4_inst_1_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_4_inst_1_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_residualblock3_instancenorm0__fwd_bn_",
      /* input */ "transformer_residual_4_conv_1",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_residualblock3_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock3_instancenorm0__fwd_bn_",
                    "transformer_residual_4_inst_1_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock3_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock3_instancenorm0__fwd_mult_gamma",
                    "transformer_residual_4_inst_1_beta"});

  nn_spec->add_relu(
      /* name */ "transformer_residualblock3_activation0",
      /* input */ "transformer_residualblock3_instancenorm0__fwd");

  nn_spec->add_padding(
      /* name */ "transformer_residualblock3_pad1",
      /* input */ "transformer_residualblock3_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_residual_4_conv_2",
      /* input */ "transformer_residualblock3_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_4_inst_2_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_4_inst_2_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_residualblock3_instancenorm1__fwd_bn_",
      /* input */ "transformer_residual_4_conv_2",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_residualblock3_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock3_instancenorm1__fwd_bn_",
                    "transformer_residual_4_inst_2_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock3_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock3_instancenorm1__fwd_mult_gamma",
                    "transformer_residual_4_inst_2_beta"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock3__plus0",
      /* inputs */ {"transformer_residualblock2__plus0",
                    "transformer_residualblock3_instancenorm1__fwd"});

  nn_spec->add_padding(
      /* name */ "transformer_residualblock4_pad0",
      /* input */ "transformer_residualblock3__plus0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_residual_5_conv_1",
      /* input */ "transformer_residualblock4_pad0",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_5_inst_1_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_5_inst_1_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_residualblock4_instancenorm0__fwd_bn_",
      /* input */ "transformer_residual_5_conv_1",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_residualblock4_instancenorm0__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock4_instancenorm0__fwd_bn_",
                    "transformer_residual_5_inst_1_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock4_instancenorm0__fwd",
      /* inputs */ {"transformer_residualblock4_instancenorm0__fwd_mult_gamma",
                    "transformer_residual_5_inst_1_beta"});

  nn_spec->add_relu(
      /* name */ "transformer_residualblock4_activation0",
      /* input */ "transformer_residualblock4_instancenorm0__fwd");

  nn_spec->add_padding(
      /* name */ "transformer_residualblock4_pad1",
      /* input */ "transformer_residualblock4_activation0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_residual_5_conv_2",
      /* input */ "transformer_residualblock4_pad1",
      /* num_output_channels */ 128,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_5_inst_2_gamma",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_residual_5_inst_2_beta",
      /* input */ "index",
      /* num_output_channels */ 128,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_residualblock4_instancenorm1__fwd_bn_",
      /* input */ "transformer_residual_5_conv_2",
      /* num_channels */ 128,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_residualblock4_instancenorm1__fwd_mult_gamma",
      /* inputs */ {"transformer_residualblock4_instancenorm1__fwd_bn_",
                    "transformer_residual_5_inst_2_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock4_instancenorm1__fwd",
      /* inputs */ {"transformer_residualblock4_instancenorm1__fwd_mult_gamma",
                    "transformer_residual_5_inst_2_beta"});

  nn_spec->add_addition(
      /* name */ "transformer_residualblock4__plus0",
      /* inputs */ {"transformer_residualblock3__plus0",
                    "transformer_residualblock4_instancenorm1__fwd"});

  nn_spec->add_upsampling(
      /* name */ "transformer_upsampling0",
      /* input */ "transformer_residualblock4__plus0",
      /* scaling_x */ 2,
      /* scaling_y */ 2);

  nn_spec->add_padding(
      /* name */ "transformer_pad3",
      /* input */ "transformer_upsampling0",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_decoding_1_conv",
      /* input */ "transformer_pad3",
      /* num_output_channels */ 64,
      /* num_kernel_channels */ 128,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_decoding_1_inst_gamma",
      /* input */ "index",
      /* num_output_channels */ 64,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_decoding_1_inst_beta",
      /* input */ "index",
      /* num_output_channels */ 64,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_instancenorm3__fwd_bn_",
      /* input */ "transformer_decoding_1_conv",
      /* num_channels */ 64,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_instancenorm3__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm3__fwd_bn_", "transformer_decoding_1_inst_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_instancenorm3__fwd",
      /* inputs */ {"transformer_instancenorm3__fwd_mult_gamma",
                    "transformer_decoding_1_inst_beta"});

  nn_spec->add_relu(
      /* name */ "transformer_activation3",
      /* input */ "transformer_instancenorm3__fwd");

  nn_spec->add_upsampling(
      /* name */ "transformer_upsampling1",
      /* input */ "transformer_activation3",
      /* scaling_x */ 2,
      /* scaling_y */ 2);

  nn_spec->add_padding(
      /* name */ "transformer_pad4",
      /* input */ "transformer_upsampling1",
      /* padding_top */ 1,
      /* padding_bottom */ 1,
      /* padding_left */ 1,
      /* padding_right */ 1);

  nn_spec->add_convolution(
      /* name */ "transformer_decoding_2_conv",
      /* input */ "transformer_pad4",
      /* num_output_channels */ 32,
      /* num_kernel_channels */ 64,
      /* kernel_height */ 3,
      /* kernel_width */ 3,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_decoding_2_inst_gamma",
      /* input */ "index",
      /* num_output_channels */ 32,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_decoding_2_inst_beta",
      /* input */ "index",
      /* num_output_channels */ 32,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_instancenorm4__fwd_bn_",
      /* input */ "transformer_decoding_2_conv",
      /* num_channels */ 32,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_instancenorm4__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm4__fwd_bn_", "transformer_decoding_2_inst_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_instancenorm4__fwd",
      /* inputs */ {"transformer_instancenorm4__fwd_mult_gamma",
                    "transformer_decoding_2_inst_beta"});

  nn_spec->add_relu(
      /* name */ "transformer_activation4",
      /* input */ "transformer_instancenorm4__fwd");

  nn_spec->add_padding(
      /* name */ "transformer_pad5",
      /* input */ "transformer_activation4",
      /* padding_top */ 4,
      /* padding_bottom */ 4,
      /* padding_left */ 4,
      /* padding_right */ 4);

  nn_spec->add_convolution(
      /* name */ "transformer_conv5",
      /* input */ "transformer_pad5",
      /* num_output_channels */ 3,
      /* num_kernel_channels */ 32,
      /* kernel_height */ 9,
      /* kernel_width */ 9,
      /* stride_height */ 1,
      /* stride_width */ 1,
      /* padding */ padding_type::VALID,
      /* weight_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_instancenorm5_gamma",
      /* input */ "index",
      /* num_output_channels */ 3,
      /* num_input_channels */ 8,
      /* weight_init_fn */ scalar_weight_initializer(1.0f),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_inner_product(
      /* name */ "transformer_instancenorm5_beta",
      /* input */ "index",
      /* num_output_channels */ 3,
      /* num_input_channels */ 8,
      /* weight_init_fn */ zero_weight_initializer(),
      /* bias_init_fn */ zero_weight_initializer());

  nn_spec->add_instancenorm(
      /* name */ "transformer_instancenorm5__fwd_bn_",
      /* input */ "transformer_conv5",
      /* num_channels */ 3,
      /* epsilon */ 9.99999974738e-06);

  nn_spec->add_multiplication(
      /* name */ "transformer_instancenorm5__fwd_mult_gamma",
      /* inputs */ {"transformer_instancenorm5__fwd_bn_", "transformer_instancenorm5_gamma"});

  nn_spec->add_addition(
      /* name */ "transformer_instancenorm5__fwd",
      /* inputs */ {"transformer_instancenorm5__fwd_mult_gamma", "transformer_instancenorm5_beta"});

  nn_spec->add_sigmoid(
      /* name */ "transformer_activation5",
      /* input */ "transformer_instancenorm5__fwd");

  nn_spec->add_scale(
      /* name */ "stylizedImage",
      /* input */ "transformer_activation5",
      /* shape_c_h_w */ {1},
      /* weight_init_fn */ scalar_weight_initializer(255.0));

  const std::string TRANSFORMER_NAME = "./transformer.mlmodel";

  CoreML::Specification::Model model;
  model.set_specificationversion(3);

  // Set Input Image
  ModelDescription* model_desc = model.mutable_description();

  FeatureDescription* input_image_feature_desc = model_desc->add_input();
  input_image_feature_desc->set_name("image");
  input_image_feature_desc->set_shortdescription("Input image");

  ImageFeatureType* input_image_feature =
      input_image_feature_desc->mutable_type()->mutable_imagetype();
  input_image_feature->set_width(256);
  input_image_feature->set_height(256);
  input_image_feature->set_colorspace(ImageFeatureType::RGB);

  // Set Input Index
  FeatureDescription* input_index_feature_desc = model_desc->add_input();
  input_index_feature_desc->set_name("index");
  input_index_feature_desc->set_shortdescription(
      "Style index array (set index I to 1.0 to enable Ith style)");

  ArrayFeatureType* array = input_index_feature_desc->mutable_type()->mutable_multiarraytype();
  array->add_shape(1);
  array->set_datatype(ArrayFeatureType::DOUBLE);

  // Set Output
  FeatureDescription* output_feature_desc = model_desc->add_output();
  output_feature_desc->set_name("stylizedImage");
  input_index_feature_desc->set_shortdescription("Stylized image");

  ImageFeatureType* output_image_feature = output_feature_desc->mutable_type()->mutable_imagetype();
  output_image_feature->set_width(256);
  output_image_feature->set_height(256);
  output_image_feature->set_colorspace(ImageFeatureType::RGB);

  model.mutable_neuralnetwork()->MergeFrom(nn_spec->get_coreml_spec());

  auto model_wrapper = std::make_shared<MLModelWrapper>(std::make_shared<CoreML::Model>(model));

  model_wrapper->save(TRANSFORMER_NAME);

  return TRANSFORMER_NAME;
}

}  // namespace style_transfer
}  // namespace turi
