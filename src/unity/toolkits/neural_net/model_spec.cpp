/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/neural_net/coreml_import.hpp>

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
using CoreML::Specification::WeightParams;

size_t multiply(size_t a, size_t b) { return a * b; }

class weight_params_float_array final: public float_array {
public:
  // Convenience function to help the compiler convert initializer lists to
  // std::vector first, before trying to resolve the std::make_shared template.
  static shared_float_array create_shared(
      std::vector<size_t> shape, WeightParams* weights) {
    return shared_float_array(
        std::make_shared<weight_params_float_array>(std::move(shape), weights));
  }

  // Adopts (destructively) the data inside weights.
  weight_params_float_array(std::vector<size_t> shape, WeightParams* weights)
    : shape_(std::move(shape))
  {
    weights_.Swap(weights);

    size_t size_from_shape =
        std::accumulate(shape_.begin(), shape_.end(), 1u, multiply);
    ASSERT_MSG(weights_.floatvalue_size() == static_cast<int>(size_from_shape),
               "WeightParams size %d inconsistent with expected size %d",
               weights_.floatvalue_size(), static_cast<int>(size_from_shape));
  }

  const float* data() const override { return weights_.floatvalue().data(); }
  size_t size() const override { return weights_.floatvalue_size(); }
  const size_t* shape() const override { return shape_.data(); }
  size_t dim() const override { return shape_.size(); }

private:
  std::vector<size_t> shape_;
  WeightParams weights_;
};

// The overloaded extract_network_params functions below traverse a CoreML spec
// proto recursively, destructively moving the WeightParams values it finds
// inside of neural networks into an output float_array_map.

void extract_network_params(const std::string& name,
                            ConvolutionLayerParams* convolution,
                            float_array_map* params_out) {
  ASSERT_EQ(2, convolution->kernelsize_size());
  const size_t n = convolution->outputchannels();
  const size_t c = convolution->kernelchannels();
  const size_t h = convolution->kernelsize(0);
  const size_t w = convolution->kernelsize(1);

  shared_float_array weights = weight_params_float_array::create_shared(
      {n, c, h, w}, convolution->mutable_weights());
  params_out->emplace(name + "_weight", std::move(weights));

  if (convolution->has_bias()) {
    shared_float_array bias = weight_params_float_array::create_shared(
        {n}, convolution->mutable_bias());
    params_out->emplace(name + "_bias", std::move(bias));
  }
}

void extract_network_params(const std::string& name,
                            BatchnormLayerParams* batch_norm,
                            float_array_map* params_out) {
  const size_t n = batch_norm->channels();

  shared_float_array gamma = weight_params_float_array::create_shared(
      {n}, batch_norm->mutable_gamma());
  params_out->emplace(name + "_gamma", std::move(gamma));

  shared_float_array beta = weight_params_float_array::create_shared(
      {n}, batch_norm->mutable_beta());
  params_out->emplace(name + "_beta", std::move(beta));

  shared_float_array mean = weight_params_float_array::create_shared(
      {n}, batch_norm->mutable_mean());
  params_out->emplace(name + "_running_mean", std::move(mean));

  shared_float_array variance = weight_params_float_array::create_shared(
      {n}, batch_norm->mutable_variance());
  params_out->emplace(name + "_running_var", std::move(variance));
}

void extract_network_params(NeuralNetworkLayer* neural_net_layer,
                            float_array_map* params_out) {
  switch (neural_net_layer->layer_case()) {
  case NeuralNetworkLayer::kConvolution:
    extract_network_params(neural_net_layer->name(),
                           neural_net_layer->mutable_convolution(), params_out);
    break;
  case NeuralNetworkLayer::kBatchnorm:
    extract_network_params(neural_net_layer->name(),
                           neural_net_layer->mutable_batchnorm(), params_out);
    break;
  default:
    break;
  }
}

void extract_network_params(NeuralNetwork* neural_net,
                            float_array_map* params_out) {
  for (NeuralNetworkLayer& layer : *neural_net->mutable_layers()) {
    extract_network_params(&layer, params_out);
  }
}

// Forward declaration necessary due to mutual recursion.
void extract_network_params(Model* model, float_array_map* params_out);

void extract_network_params(Pipeline* pipeline, float_array_map* params_out) {
  for (Model& model : *pipeline->mutable_models()) {
    extract_network_params(&model, params_out);
  }
}

void extract_network_params(Model* model, float_array_map* params_out) {
  switch (model->Type_case()) {
  case Model::kNeuralNetwork:
    extract_network_params(model->mutable_neuralnetwork(), params_out);
    break;
  case Model::kPipeline:
    extract_network_params(model->mutable_pipeline(), params_out);
    break;
  default:
    break;
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

float_array_map extract_network_params(Model* model) {
  float_array_map result;
  extract_network_params(model, &result);
  return result;
}

float_array_map load_network_params(const std::string& mlmodel_path) {
  CoreML::Specification::Model mlmodel;

  // Read file.
  std::vector<char> mlmodel_buffer = load_file(mlmodel_path);

  // Parse into CoreML spec.
  if (!mlmodel.ParseFromArray(mlmodel_buffer.data(),
                              static_cast<int>(mlmodel_buffer.size()))) {
    log_and_throw("Error parsing CoreML specification from " + mlmodel_path);
  }

  // Rip out the parameters.
  return extract_network_params(&mlmodel);
}

}  // neural_net
}  // turi
