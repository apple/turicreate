/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_coreml_import

#include <unity/toolkits/neural_net/coreml_import.hpp>

#include <boost/test/unit_test.hpp>
#include <unity/toolkits/coreml_export/mlmodel_include.hpp>
#include <util/test_macros.hpp>

using turi::neural_net::extract_network_params;
using turi::neural_net::float_array_map;

using CoreML::Specification::Model;

BOOST_AUTO_TEST_CASE(test_extract_empty) {
  Model model;
  float_array_map params = extract_network_params(&model);
  TS_ASSERT(params.empty());
}

BOOST_AUTO_TEST_CASE(test_extract_conv_params) {
  // Build a CoreML spec with just a single conv layer.
  Model model;
  auto* conv_layer = model.mutable_neuralnetwork()->add_layers();
  conv_layer->set_name("conv_test");
  auto* conv_params = conv_layer->mutable_convolution();
  conv_params->set_outputchannels(2);  // N
  conv_params->set_kernelchannels(3);  // C
  conv_params->add_kernelsize(4);      // H
  conv_params->add_kernelsize(5);      // W
  const size_t size = 2*3*4*5;
  auto* weights = conv_params->mutable_weights();
  for (size_t i = 0; i < size; ++i) {
    weights->add_floatvalue(100.f + i);
  }

  // Extract the parameters from the spec.
  float_array_map params = extract_network_params(&model);

  // The result should have just one float array.
  TS_ASSERT_EQUALS(params.size(), 1);
  auto float_array = params["conv_test_weight"];

  // Shape must be [N, C, H, W], which is [2, 3, 4, 5]
  TS_ASSERT_EQUALS(float_array.dim(), 4);
  for (size_t i = 0; i < 4; ++i) {
    TS_ASSERT_EQUALS(float_array.shape()[i], i + 2);
  }

  // Verify the data was extracted intact.
  TS_ASSERT_EQUALS(float_array.size(), size);
  for (size_t i = 0; i < size; ++i) {
    TS_ASSERT_EQUALS(float_array.data()[i], 100.f + i);
  }
}

BOOST_AUTO_TEST_CASE(test_extract_conv_params_invalid) {
  Model model;
  auto* conv_layer = model.mutable_neuralnetwork()->add_layers();
  conv_layer->set_name("conv_test");
  conv_layer->mutable_convolution();
  // The default ConvolutionLayerParams value is not valid.

  TS_ASSERT_THROWS_ANYTHING(extract_network_params(&model));
}

BOOST_AUTO_TEST_CASE(test_extract_batchnorm_params) {
  // Build a CoreML spec with just a single batchnorm layer.
  Model model;
  auto* batchnorm_layer = model.mutable_neuralnetwork()->add_layers();
  batchnorm_layer->set_name("batchnorm_test");
  auto* batchnorm_params = batchnorm_layer->mutable_batchnorm();
  batchnorm_params->set_channels(1);
  batchnorm_params->mutable_gamma()->add_floatvalue(2.0f);
  batchnorm_params->mutable_beta()->add_floatvalue(3.0f);
  batchnorm_params->mutable_mean()->add_floatvalue(4.0f);
  batchnorm_params->mutable_variance()->add_floatvalue(5.0f);

  // Extract the parameters from the spec.
  float_array_map params = extract_network_params(&model);

  // The result should have four float arrays.
  TS_ASSERT_EQUALS(params.size(), 4);

  TS_ASSERT_EQUALS(params["batchnorm_test_gamma"].dim(), 1);
  TS_ASSERT_EQUALS(params["batchnorm_test_gamma"].shape()[0], 1);
  TS_ASSERT_EQUALS(params["batchnorm_test_gamma"].data()[0], 2.0f);

  TS_ASSERT_EQUALS(params["batchnorm_test_beta"].dim(), 1);
  TS_ASSERT_EQUALS(params["batchnorm_test_beta"].shape()[0], 1);
  TS_ASSERT_EQUALS(params["batchnorm_test_beta"].data()[0], 3.0f);

  TS_ASSERT_EQUALS(params["batchnorm_test_running_mean"].dim(), 1);
  TS_ASSERT_EQUALS(params["batchnorm_test_running_mean"].shape()[0], 1);
  TS_ASSERT_EQUALS(params["batchnorm_test_running_mean"].data()[0], 4.0f);

  TS_ASSERT_EQUALS(params["batchnorm_test_running_var"].dim(), 1);
  TS_ASSERT_EQUALS(params["batchnorm_test_running_var"].shape()[0], 1);
  TS_ASSERT_EQUALS(params["batchnorm_test_running_var"].data()[0], 5.0f);
}

BOOST_AUTO_TEST_CASE(test_extract_pipeline) {
  // Build a CoreML spec with just a single conv layer, embedded inside a
  // pipeline model.
  Model model;
  auto* conv_layer = model.mutable_pipeline()->add_models()
      ->mutable_neuralnetwork()->add_layers();
  conv_layer->set_name("conv_test");
  auto* conv_params = conv_layer->mutable_convolution();
  conv_params->set_outputchannels(2);  // N
  conv_params->set_kernelchannels(3);  // C
  conv_params->add_kernelsize(4);      // H
  conv_params->add_kernelsize(5);      // W
  const size_t size = 2*3*4*5;
  auto* weights = conv_params->mutable_weights();
  for (size_t i = 0; i < size; ++i) {
    weights->add_floatvalue(100.f + i);
  }

  // Extract the parameters from the spec.
  float_array_map params = extract_network_params(&model);

  // The result should have just one float array.
  TS_ASSERT_EQUALS(params.size(), 1);
  auto float_array = params["conv_test_weight"];
  TS_ASSERT_EQUALS(float_array.size(), size);
}

