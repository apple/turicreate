/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/neural_net/model_spec.hpp>

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <vector>

#include <logger/assertions.hpp>
#include <logger/logger.hpp>
#include <unity/toolkits/coreml_export/mlmodel_include.hpp>

namespace turi {
namespace neural_net {

namespace {

using CoreML::Specification::BatchnormLayerParams;
using CoreML::Specification::ConvolutionLayerParams;
using CoreML::Specification::Model;
using CoreML::Specification::NeuralNetwork;
using CoreML::Specification::NeuralNetworkLayer;
using CoreML::Specification::Pipeline;
using CoreML::Specification::SamePadding;
using CoreML::Specification::WeightParams;

size_t multiply(size_t a, size_t b) { return a * b; }

class weight_params_float_array final: public float_array {
public:

  // Convenience function to help the compiler convert initializer lists to
  // std::vector first, before trying to resolve the std::make_shared template.
  static shared_float_array create_shared(
      std::vector<size_t> shape, const WeightParams& weights) {

    return shared_float_array(
        std::make_shared<weight_params_float_array>(std::move(shape), weights));
  }

  weight_params_float_array(std::vector<size_t> shape,
                            const WeightParams& weights)
    : shape_(std::move(shape)), weights_(weights)
  {
    size_t size_from_shape =
        std::accumulate(shape_.begin(), shape_.end(), 1u, multiply);

    if (weights_.floatvalue_size() != static_cast<int>(size_from_shape)) {
      std::stringstream ss;
      ss << "WeightParams size " << weights_.floatvalue_size()
         << " inconsistent with expected size " << size_from_shape;
      log_and_throw(ss.str());
    }
  }

  const float* data() const override { return weights_.floatvalue().data(); }
  size_t size() const override { return weights_.floatvalue_size(); }
  const size_t* shape() const override { return shape_.data(); }
  size_t dim() const override { return shape_.size(); }

private:
  std::vector<size_t> shape_;
  const WeightParams& weights_;
};

void update_weight_params(const std::string& name, const float_array& value,
                          WeightParams* weights) {

  if (weights->floatvalue_size() != static_cast<int>(value.size())) {
    std::stringstream ss;
    ss << "float_array " << name << " has size " << value.size()
       << " inconsistent with expected size " << weights->floatvalue_size();
    log_and_throw(ss.str());
  }

  std::copy(value.data(), value.data() + value.size(),
            weights->mutable_floatvalue()->begin());
}

// The overloaded wrap_network_params functions below traverse a CoreML spec
// proto recursively, storing references to the WeightParams values it finds
// (inside of neural networks) into an output float_array_map.
//
// The corresponding update_network_params functions traverse a CoreML spec
// proto recursively, overwriting the WeightParams values with the float_array
// values found in the provided float_array_map.

void wrap_network_params(const std::string& name,
                         const ConvolutionLayerParams& convolution,
                         float_array_map* params_out) {

  ASSERT_EQ(2, convolution.kernelsize_size());
  const size_t n = convolution.outputchannels();
  const size_t c = convolution.kernelchannels();
  const size_t h = convolution.kernelsize(0);
  const size_t w = convolution.kernelsize(1);

  shared_float_array weights = weight_params_float_array::create_shared(
      {n, c, h, w}, convolution.weights());
  params_out->emplace(name + "_weight", std::move(weights));

  if (convolution.has_bias()) {
    shared_float_array bias = weight_params_float_array::create_shared(
        {n}, convolution.bias());
    params_out->emplace(name + "_bias", std::move(bias));
  }
}

void update_network_params(const std::string& name,
                           const float_array_map& params,
                           ConvolutionLayerParams* convolution) {

  std::string key = name + "_weight";
  auto it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second, convolution->mutable_weights());
  }

  if (convolution->has_bias()) {
    key = name + "_bias";
    it = params.find(key);
    if (it != params.end()) {
      update_weight_params(key, it->second, convolution->mutable_bias());
    }
  }
}

void wrap_network_params(const std::string& name,
                         const BatchnormLayerParams& batch_norm,
                         float_array_map* params_out) {

  const size_t n = batch_norm.channels();

  shared_float_array gamma = weight_params_float_array::create_shared(
      {n}, batch_norm.gamma());
  params_out->emplace(name + "_gamma", std::move(gamma));

  shared_float_array beta = weight_params_float_array::create_shared(
      {n}, batch_norm.beta());
  params_out->emplace(name + "_beta", std::move(beta));

  shared_float_array mean = weight_params_float_array::create_shared(
      {n}, batch_norm.mean());
  params_out->emplace(name + "_running_mean", std::move(mean));

  shared_float_array variance = weight_params_float_array::create_shared(
      {n}, batch_norm.variance());
  params_out->emplace(name + "_running_var", std::move(variance));
}

void update_network_params(const std::string& name,
                           const float_array_map& params,
                           BatchnormLayerParams* batch_norm) {

  std::string key = name + "_gamma";
  auto it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second, batch_norm->mutable_gamma());
  }

  key = name + "_beta";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second, batch_norm->mutable_beta());
  }

  key = name + "_running_mean";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second, batch_norm->mutable_mean());
  }

  key = name + "_running_var";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second, batch_norm->mutable_variance());
  }
}

void wrap_network_params(const NeuralNetworkLayer& neural_net_layer,
                         float_array_map* params_out) {

  switch (neural_net_layer.layer_case()) {
  case NeuralNetworkLayer::kConvolution:
    wrap_network_params(neural_net_layer.name(),
                        neural_net_layer.convolution(), params_out);
    break;
  case NeuralNetworkLayer::kBatchnorm:
    wrap_network_params(neural_net_layer.name(),
                        neural_net_layer.batchnorm(), params_out);
    break;
  default:
    break;
  }
}

void update_network_params(const float_array_map& params,
                           NeuralNetworkLayer* neural_net_layer) {

  switch (neural_net_layer->layer_case()) {
  case NeuralNetworkLayer::kConvolution:
    update_network_params(neural_net_layer->name(), params,
                          neural_net_layer->mutable_convolution());
    break;
  case NeuralNetworkLayer::kBatchnorm:
    update_network_params(neural_net_layer->name(), params,
                          neural_net_layer->mutable_batchnorm());
    break;
  default:
    break;
  }
}

void wrap_network_params(const NeuralNetwork& neural_net,
                         float_array_map* params_out) {

  for (const NeuralNetworkLayer& layer : neural_net.layers()) {
    wrap_network_params(layer, params_out);
  }
}

void update_network_params(const float_array_map& params,
                           NeuralNetwork* neural_net) {

  for (NeuralNetworkLayer& layer : *neural_net->mutable_layers()) {
    update_network_params(params, &layer);
  }
}

std::vector<char> load_file(const std::string& path) {

  // Open file in binary mode at end of file.
  std::ifstream input_stream(path, std::ios::binary | std::ios::ate);
  if (!input_stream) {
    log_and_throw("Error opening " + path);
  }

  // Allocate buffer.
  std::vector<char> buffer(input_stream.tellg());

  // Read file.
  input_stream.seekg(0);
  if (!input_stream.read(buffer.data(), buffer.size())) {
    log_and_throw("Error reading " + path);
  }

  return buffer;
}

}  // namespace

model_spec::model_spec(): impl_(new NeuralNetwork) {}

model_spec::model_spec(const NeuralNetwork& nn_model)
  : impl_(new NeuralNetwork(nn_model))
{}

model_spec::model_spec(const std::string& mlmodel_path)
  : impl_(new NeuralNetwork)
{

  // Read file.
  // TODO: Parse from an input stream, without loading entire file into memory
  // at once. Consider using existing I/O from the CoreML::Model class.
  std::vector<char> mlmodel_buffer = load_file(mlmodel_path);

  // Parse into CoreML spec.
  Model mlmodel;
  if (!mlmodel.ParseFromArray(mlmodel_buffer.data(),
                               static_cast<int>(mlmodel_buffer.size()))) {

    log_and_throw("Error parsing CoreML specification from " + mlmodel_path);
  }

  // Swap the parsed NeuralNetwork into our impl value.
  impl_->Swap(mlmodel.mutable_neuralnetwork());
}

model_spec::~model_spec() = default;

float_array_map model_spec::export_params_view() const {
  float_array_map result;
  wrap_network_params(*impl_, &result);
  return result;
}

void model_spec::update_params(const float_array_map& weights) {
  update_network_params(weights, impl_.get());
}

bool model_spec::has_layer_output(const std::string& layer_name) const {

  for (const NeuralNetworkLayer& layer : impl_->layers()) {

    for (const std::string& output : layer.output()) {

      if (output == layer_name) return true;
    }
  }

  return false;
}

void model_spec::add_convolution(
    const std::string& name, const std::string& input,
    size_t num_output_channels, size_t num_kernel_channels, size_t kernel_size,
    weight_initializer weight_initializer_fn,
    weight_initializer bias_initializer_fn) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  ConvolutionLayerParams* params = layer->mutable_convolution();
  params->set_outputchannels(num_output_channels);
  params->set_kernelchannels(num_kernel_channels);
  params->set_ngroups(1);
  params->add_kernelsize(kernel_size);
  params->add_kernelsize(kernel_size);
  params->add_stride(1);
  params->add_stride(1);
  params->add_dilationfactor(1);
  params->add_dilationfactor(1);
  params->mutable_same()->set_asymmetrymode(SamePadding::TOP_LEFT_HEAVY);

  size_t weights_size =
      num_output_channels * num_kernel_channels * kernel_size * kernel_size;
  params->mutable_weights()->mutable_floatvalue()->Resize(
      static_cast<int>(weights_size), 0.f);
  float* weights =
      params->mutable_weights()->mutable_floatvalue()->mutable_data();
  weight_initializer_fn(weights, weights + weights_size);

  if (bias_initializer_fn) {
    params->set_hasbias(true);
    params->mutable_bias()->mutable_floatvalue()->Resize(
        static_cast<int>(num_output_channels), 0.f);
    float* bias = params->mutable_bias()->mutable_floatvalue()->mutable_data();
    bias_initializer_fn(bias, bias + num_output_channels);
  }
}

void model_spec::add_batchnorm(
    const std::string& name, const std::string& input, size_t num_channels,
    float epsilon) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  BatchnormLayerParams* params = layer->mutable_batchnorm();
  params->set_channels(num_channels);
  params->set_epsilon(epsilon);

  int size = static_cast<int>(num_channels);
  params->mutable_gamma()->mutable_floatvalue()->Resize(size, 1.f);
  params->mutable_beta()->mutable_floatvalue()->Resize(size, 0.f);
  params->mutable_mean()->mutable_floatvalue()->Resize(size, 0.f);
  params->mutable_variance()->mutable_floatvalue()->Resize(size, 1.f);
}

void model_spec::add_leakyrelu(const std::string& name,
                               const std::string& input, float alpha) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  layer->mutable_activation()->mutable_leakyrelu()->set_alpha(alpha);
}

}  // neural_net
}  // turi
