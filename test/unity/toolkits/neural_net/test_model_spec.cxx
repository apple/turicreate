/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_model_spec

#include <ml/neural_net/model_spec.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>

#include <boost/test/unit_test.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <core/util/test_macros.hpp>

namespace turi {
namespace neural_net {
namespace {

using CoreML::Specification::NeuralNetwork;
using CoreML::Specification::NeuralNetworkLayer;

BOOST_AUTO_TEST_CASE(test_export_empty) {

  NeuralNetwork nn_model;
  model_spec nn_spec(nn_model);
  float_array_map params = nn_spec.export_params_view();
  TS_ASSERT(params.empty());
}

BOOST_AUTO_TEST_CASE(test_export_conv_params) {

  // Build a CoreML spec with just a single conv layer.
  NeuralNetwork nn_model;
  auto* conv_layer = nn_model.add_layers();
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
  model_spec nn_spec(nn_model);
  float_array_map params = nn_spec.export_params_view();

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

BOOST_AUTO_TEST_CASE(test_export_conv_params_invalid) {

  NeuralNetwork nn_model;
  auto* conv_layer = nn_model.add_layers();
  conv_layer->set_name("conv_test");
  conv_layer->mutable_convolution();
  // The default ConvolutionLayerParams value is not valid.

  model_spec nn_spec(nn_model);
  TS_ASSERT_THROWS_ANYTHING(nn_spec.export_params_view());
}

BOOST_AUTO_TEST_CASE(test_export_batchnorm_params) {

  // Build a CoreML spec with just a single batchnorm layer.
  NeuralNetwork nn_model;
  auto* batchnorm_layer = nn_model.add_layers();
  batchnorm_layer->set_name("batchnorm_test");
  auto* batchnorm_params = batchnorm_layer->mutable_batchnorm();
  batchnorm_params->set_channels(1);
  batchnorm_params->mutable_gamma()->add_floatvalue(2.0f);
  batchnorm_params->mutable_beta()->add_floatvalue(3.0f);
  batchnorm_params->mutable_mean()->add_floatvalue(4.0f);
  batchnorm_params->mutable_variance()->add_floatvalue(5.0f);

  // Extract the parameters from the spec.
  model_spec nn_spec(nn_model);
  float_array_map params = nn_spec.export_params_view();

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

BOOST_AUTO_TEST_CASE(test_add_convolution) {

  // Add an arbitrary convolution layer to an empty model spec.
  model_spec nn_spec;
  int weights_size = 16*8*5*5;
  auto weight_init_fn = [weights_size](float* w, float* w_end) {
    TS_ASSERT_EQUALS(w_end - w, weights_size);
    for (int i = 0; i < weights_size; ++i) {
      w[i] = static_cast<float>(i);
    }
  };
  nn_spec.add_convolution("test_name", "test_input", 16, 8, 5, 5, 1, 1,
                          model_spec::padding_type::SAME, weight_init_fn);

  // Verify the resulting NeuralNetworkLayer value.
  TS_ASSERT_EQUALS(nn_spec.get_coreml_spec().layers_size(), 1);
  const NeuralNetworkLayer& layer = nn_spec.get_coreml_spec().layers(0);

  TS_ASSERT_EQUALS(layer.name(), "test_name");
  TS_ASSERT_EQUALS(layer.input_size(), 1);
  TS_ASSERT_EQUALS(layer.input(0), "test_input");
  TS_ASSERT_EQUALS(layer.output_size(), 1);
  TS_ASSERT_EQUALS(layer.output(0), "test_name");

  TS_ASSERT_EQUALS(layer.convolution().outputchannels(), 16);
  TS_ASSERT_EQUALS(layer.convolution().kernelchannels(), 8);
  TS_ASSERT_EQUALS(layer.convolution().ngroups(), 1);
  TS_ASSERT(layer.convolution().has_same());
  TS_ASSERT_EQUALS(layer.convolution().same().asymmetrymode(),
                   CoreML::Specification::SamePadding::TOP_LEFT_HEAVY);

  // These repeated fields must all have length 2.
  TS_ASSERT_EQUALS(layer.convolution().kernelsize_size(), 2);
  TS_ASSERT_EQUALS(layer.convolution().stride_size(), 2);
  TS_ASSERT_EQUALS(layer.convolution().dilationfactor_size(), 2);

  // Check each value of these repeated fields.
  TS_ASSERT_EQUALS(layer.convolution().kernelsize(0), 5);
  TS_ASSERT_EQUALS(layer.convolution().kernelsize(1), 5);
  TS_ASSERT_EQUALS(layer.convolution().stride(0), 1);
  TS_ASSERT_EQUALS(layer.convolution().stride(1), 1);
  TS_ASSERT_EQUALS(layer.convolution().dilationfactor(0), 1);
  TS_ASSERT_EQUALS(layer.convolution().dilationfactor(1), 1);

  // Verify the contents of the weights
  const auto& conv_weights = layer.convolution().weights().floatvalue();
  TS_ASSERT_EQUALS(conv_weights.size(), weights_size);
  for (int i = 0; i < weights_size; ++i) {
    TS_ASSERT_EQUALS(conv_weights[i], static_cast<float>(i));
  }

  // Update the weights, adding 7.f to each value.
  std::vector<float> updated_weights(conv_weights.begin(), conv_weights.end());
  std::for_each(updated_weights.begin(), updated_weights.end(),
                [](float& x) { x += 7.f; });
  nn_spec.update_params({
      {"test_name_weight",
       shared_float_array::wrap(updated_weights, {16, 8, 5, 5})}
  });

  // Verify the updated weights.
  TS_ASSERT_EQUALS(conv_weights.size(), weights_size);
  for (int i = 0; i < weights_size; ++i) {
    TS_ASSERT_EQUALS(conv_weights[i], static_cast<float>(i) + 7.f);
  }
}

BOOST_AUTO_TEST_CASE(test_add_batchnorm) {

  // Add an arbitrary batch layer to an empty model spec.
  model_spec nn_spec;
  nn_spec.add_batchnorm("test_name", "test_input", 16, 0.125f);

  // Verify the resulting NeuralNetworkLayer value.
  TS_ASSERT_EQUALS(nn_spec.get_coreml_spec().layers_size(), 1);
  const NeuralNetworkLayer& layer = nn_spec.get_coreml_spec().layers(0);

  TS_ASSERT_EQUALS(layer.name(), "test_name");
  TS_ASSERT_EQUALS(layer.input_size(), 1);
  TS_ASSERT_EQUALS(layer.input(0), "test_input");
  TS_ASSERT_EQUALS(layer.output_size(), 1);
  TS_ASSERT_EQUALS(layer.output(0), "test_name");

  TS_ASSERT_EQUALS(layer.batchnorm().channels(), 16);
  TS_ASSERT_EQUALS(layer.batchnorm().epsilon(), 0.125f);

  // These repeated fields must all have length 16.
  TS_ASSERT_EQUALS(layer.batchnorm().gamma().floatvalue_size(), 16);
  TS_ASSERT_EQUALS(layer.batchnorm().beta().floatvalue_size(), 16);
  TS_ASSERT_EQUALS(layer.batchnorm().mean().floatvalue_size(), 16);
  TS_ASSERT_EQUALS(layer.batchnorm().variance().floatvalue_size(), 16);

  // Check each value of these repeated fields.
  TS_ASSERT(std::all_of(layer.batchnorm().gamma().floatvalue().begin(),
                        layer.batchnorm().gamma().floatvalue().end(),
                        [](float f) { return f == 1.f; }));
  TS_ASSERT(std::all_of(layer.batchnorm().beta().floatvalue().begin(),
                        layer.batchnorm().beta().floatvalue().end(),
                        [](float f) { return f == 0.f; }));
  TS_ASSERT(std::all_of(layer.batchnorm().mean().floatvalue().begin(),
                        layer.batchnorm().mean().floatvalue().end(),
                        [](float f) { return f == 0.f; }));
  TS_ASSERT(std::all_of(layer.batchnorm().variance().floatvalue().begin(),
                        layer.batchnorm().variance().floatvalue().end(),
                        [](float f) { return f == 1.f; }));

  // Update the beta values to 0.5f.
  std::vector<float> updated_beta(16, 0.5f);
  nn_spec.update_params({
      {"test_name_beta", shared_float_array::wrap(updated_beta, {16})}
  });

  // Check the update beta values.
  TS_ASSERT(std::all_of(layer.batchnorm().beta().floatvalue().begin(),
                        layer.batchnorm().beta().floatvalue().end(),
                        [](float f) { return f == 0.5f; }));
}

BOOST_AUTO_TEST_CASE(test_add_leakyrelu) {

  // Add an arbitrary leaky ReLU layer to an empty model spec.
  model_spec nn_spec;
  nn_spec.add_leakyrelu("test_name", "test_input", 0.125f);

  // Verify the resulting NeuralNetworkLayer value.
  TS_ASSERT_EQUALS(nn_spec.get_coreml_spec().layers_size(), 1);
  const NeuralNetworkLayer& layer = nn_spec.get_coreml_spec().layers(0);

  TS_ASSERT_EQUALS(layer.name(), "test_name");
  TS_ASSERT_EQUALS(layer.input_size(), 1);
  TS_ASSERT_EQUALS(layer.input(0), "test_input");
  TS_ASSERT_EQUALS(layer.output_size(), 1);
  TS_ASSERT_EQUALS(layer.output(0), "test_name");

  TS_ASSERT_EQUALS(layer.activation().leakyrelu().alpha(), 0.125f);
}

}  // namespace
}  // namespace neural_net
}  // namespace turi
