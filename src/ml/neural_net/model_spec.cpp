/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/neural_net/model_spec.hpp>

#include <algorithm>
#include <fstream>
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
using CoreML::Specification::InnerProductLayerParams;
using CoreML::Specification::Model;
using CoreML::Specification::NeuralNetwork;
using CoreML::Specification::NeuralNetworkLayer;
using CoreML::Specification::SamePadding;
using CoreML::Specification::UniDirectionalLSTMLayerParams;
using CoreML::Specification::WeightParams;

size_t multiply(size_t a, size_t b) { return a * b; }

class weight_params_float_array final: public float_array {
public:

  // Convenience function to help the compiler convert initializer lists to
  // std::vector first, before trying to resolve the std::make_shared template.
  static shared_float_array create_view(
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

  shared_float_array weights = weight_params_float_array::create_view(
      {n, c, h, w}, convolution.weights());
  params_out->emplace(name + "_weight", std::move(weights));

  if (convolution.has_bias()) {
    shared_float_array bias = weight_params_float_array::create_view(
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
                         const InnerProductLayerParams& inner_product,
                         float_array_map* params_out) {

  const size_t n = inner_product.outputchannels();
  const size_t c = inner_product.inputchannels();

  shared_float_array weights = weight_params_float_array::create_view(
      {n, c, 1, 1}, inner_product.weights());
  params_out->emplace(name + "_weight", std::move(weights));

  if (inner_product.has_bias()) {
    shared_float_array bias = weight_params_float_array::create_view(
        {n}, inner_product.bias());
    params_out->emplace(name + "_bias", std::move(bias));
  }
}

void update_network_params(const std::string& name,
                           const float_array_map& params,
                           InnerProductLayerParams* inner_product) {

  std::string key = name + "_weight";
  auto it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second, inner_product->mutable_weights());
  }

  if (inner_product->has_bias()) {
    key = name + "_bias";
    it = params.find(key);
    if (it != params.end()) {
      update_weight_params(key, it->second, inner_product->mutable_bias());
    }
  }
}

void wrap_network_params(const std::string& name,
                         const BatchnormLayerParams& batch_norm,
                         float_array_map* params_out) {

  const size_t n = batch_norm.channels();

  shared_float_array gamma = weight_params_float_array::create_view(
      {n}, batch_norm.gamma());
  params_out->emplace(name + "_gamma", std::move(gamma));

  shared_float_array beta = weight_params_float_array::create_view(
      {n}, batch_norm.beta());
  params_out->emplace(name + "_beta", std::move(beta));

  shared_float_array mean = weight_params_float_array::create_view(
      {n}, batch_norm.mean());
  params_out->emplace(name + "_running_mean", std::move(mean));

  shared_float_array variance = weight_params_float_array::create_view(
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

void wrap_network_params(const std::string& name,
                         const UniDirectionalLSTMLayerParams& lstm,
                         float_array_map* params_out) {

  const size_t n = lstm.outputvectorsize();
  const size_t c = lstm.inputvectorsize();

  shared_float_array i2h_i_weight = weight_params_float_array::create_view(
      {n, c}, lstm.weightparams().inputgateweightmatrix());
  params_out->emplace(name + "_i2h_i_weight", std::move(i2h_i_weight));

  shared_float_array i2h_f_weight = weight_params_float_array::create_view(
      {n, c}, lstm.weightparams().forgetgateweightmatrix());
  params_out->emplace(name + "_i2h_f_weight", std::move(i2h_f_weight));

  shared_float_array i2h_c_weight = weight_params_float_array::create_view(
      {n, c}, lstm.weightparams().blockinputweightmatrix());
  params_out->emplace(name + "_i2h_c_weight", std::move(i2h_c_weight));

  shared_float_array i2h_o_weight = weight_params_float_array::create_view(
      {n, c}, lstm.weightparams().outputgateweightmatrix());
  params_out->emplace(name + "_i2h_o_weight", std::move(i2h_o_weight));

  shared_float_array h2h_i_weight = weight_params_float_array::create_view(
      {n, n}, lstm.weightparams().inputgaterecursionmatrix());
  params_out->emplace(name + "_h2h_i_weight", std::move(h2h_i_weight));

  shared_float_array h2h_f_weight = weight_params_float_array::create_view(
      {n, n}, lstm.weightparams().forgetgaterecursionmatrix());
  params_out->emplace(name + "_h2h_f_weight", std::move(h2h_f_weight));

  shared_float_array h2h_c_weight = weight_params_float_array::create_view(
      {n, n}, lstm.weightparams().blockinputrecursionmatrix());
  params_out->emplace(name + "_h2h_c_weight", std::move(h2h_c_weight));

  shared_float_array h2h_o_weight = weight_params_float_array::create_view(
      {n, n}, lstm.weightparams().outputgaterecursionmatrix());
  params_out->emplace(name + "_h2h_o_weight", std::move(h2h_o_weight));

  shared_float_array h2h_i_bias = weight_params_float_array::create_view(
      {n}, lstm.weightparams().inputgatebiasvector());
  params_out->emplace(name + "_h2h_i_bias", std::move(h2h_i_bias));

  shared_float_array h2h_f_bias = weight_params_float_array::create_view(
      {n}, lstm.weightparams().forgetgatebiasvector());
  params_out->emplace(name + "_h2h_f_bias", std::move(h2h_f_bias));

  shared_float_array h2h_c_bias = weight_params_float_array::create_view(
      {n}, lstm.weightparams().blockinputbiasvector());
  params_out->emplace(name + "_h2h_c_bias", std::move(h2h_c_bias));

  shared_float_array h2h_o_bias = weight_params_float_array::create_view(
      {n}, lstm.weightparams().outputgatebiasvector());
  params_out->emplace(name + "_h2h_o_bias", std::move(h2h_o_bias));
}

void update_network_params(const std::string& name,
                           const float_array_map& params,
                           UniDirectionalLSTMLayerParams* lstm) {

  auto* lstm_params = lstm->mutable_weightparams();

  std::string key = name + "_i2h_i_weight";
  auto it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_inputgateweightmatrix());
  }

  key = name + "_i2h_f_weight";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_forgetgateweightmatrix());
  }

  key = name + "_i2h_c_weight";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_blockinputweightmatrix());
  }

  key = name + "_i2h_o_weight";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_outputgateweightmatrix());
  }

  key = name + "_h2h_i_weight";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_inputgaterecursionmatrix());
  }

  key = name + "_h2h_f_weight";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_forgetgaterecursionmatrix());
  }

  key = name + "_h2h_c_weight";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_blockinputrecursionmatrix());
  }

  key = name + "_h2h_o_weight";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_outputgaterecursionmatrix());
  }

  key = name + "_h2h_i_bias";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_inputgatebiasvector());
  }

  key = name + "_h2h_f_bias";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_forgetgatebiasvector());
  }

  key = name + "_h2h_c_bias";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_blockinputbiasvector());
  }

  key = name + "_h2h_o_bias";
  it = params.find(key);
  if (it != params.end()) {
    update_weight_params(key, it->second,
                         lstm_params->mutable_outputgatebiasvector());
  }
}

void wrap_network_params(const NeuralNetworkLayer& neural_net_layer,
                         float_array_map* params_out) {

  switch (neural_net_layer.layer_case()) {
  case NeuralNetworkLayer::kConvolution:
    wrap_network_params(neural_net_layer.name(),
                        neural_net_layer.convolution(), params_out);
    break;
  case NeuralNetworkLayer::kInnerProduct:
    wrap_network_params(neural_net_layer.name(),
                        neural_net_layer.innerproduct(), params_out);
    break;
  case NeuralNetworkLayer::kBatchnorm:
    wrap_network_params(neural_net_layer.name(),
                        neural_net_layer.batchnorm(), params_out);
    break;
  case NeuralNetworkLayer::kUniDirectionalLSTM:
    wrap_network_params(neural_net_layer.name(),
                        neural_net_layer.unidirectionallstm(), params_out);
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
  case NeuralNetworkLayer::kInnerProduct:
    update_network_params(neural_net_layer->name(), params,
                          neural_net_layer->mutable_innerproduct());
    break;
  case NeuralNetworkLayer::kBatchnorm:
    update_network_params(neural_net_layer->name(), params,
                          neural_net_layer->mutable_batchnorm());
    break;
  case NeuralNetworkLayer::kUniDirectionalLSTM:
    update_network_params(neural_net_layer->name(), params,
                          neural_net_layer->mutable_unidirectionallstm());
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

void init_weight_params(WeightParams* params, size_t size,
                        const weight_initializer& weight_init_fn) {

  params->mutable_floatvalue()->Resize(static_cast<int>(size), 0.f);
  float* weights = params->mutable_floatvalue()->mutable_data();
  weight_init_fn(weights, weights + size);
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

std::unique_ptr<NeuralNetwork> model_spec::move_coreml_spec() && {
  return std::move(impl_);
}

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

void model_spec::add_relu(const std::string& name, const std::string& input) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  layer->mutable_activation()->mutable_relu();
}

void model_spec::add_leakyrelu(const std::string& name,
                               const std::string& input, float alpha) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  layer->mutable_activation()->mutable_leakyrelu()->set_alpha(alpha);
}

void model_spec::add_sigmoid(const std::string& name,
                             const std::string& input) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  layer->mutable_activation()->mutable_sigmoid();
}

void model_spec::add_convolution(
    const std::string& name, const std::string& input,
    size_t num_output_channels, size_t num_kernel_channels,
    size_t kernel_height, size_t kernel_width, size_t stride_h, size_t stride_w,
    padding_type padding, weight_initializer weight_initializer_fn,
    weight_initializer bias_initializer_fn) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  ConvolutionLayerParams* params = layer->mutable_convolution();
  params->set_outputchannels(num_output_channels);
  params->set_kernelchannels(num_kernel_channels);
  params->set_ngroups(1);
  params->add_kernelsize(kernel_height);
  params->add_kernelsize(kernel_width);
  params->add_stride(stride_h);
  params->add_stride(stride_w);
  params->add_dilationfactor(1);
  params->add_dilationfactor(1);
  switch (padding) {
  case padding_type::VALID:
    params->mutable_valid()->mutable_paddingamounts()->add_borderamounts();
    params->mutable_valid()->mutable_paddingamounts()->add_borderamounts();
    break;
  case padding_type::SAME:
    params->mutable_same()->set_asymmetrymode(SamePadding::TOP_LEFT_HEAVY);
    break;
  }

  size_t weights_size =
      num_output_channels * num_kernel_channels * kernel_height * kernel_width;
  init_weight_params(params->mutable_weights(), weights_size,
                     weight_initializer_fn);

  if (bias_initializer_fn) {
    params->set_hasbias(true);
    init_weight_params(params->mutable_bias(), num_output_channels,
                       bias_initializer_fn);
  }
}

void model_spec::add_inner_product(
    const std::string& name, const std::string& input,
    size_t num_output_channels, size_t num_input_channels,
    weight_initializer weight_initializer_fn,
    weight_initializer bias_initializer_fn) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  InnerProductLayerParams* params = layer->mutable_innerproduct();
  params->set_outputchannels(num_output_channels);
  params->set_inputchannels(num_input_channels);
  size_t weights_size = num_output_channels * num_input_channels;
  init_weight_params(params->mutable_weights(), weights_size,
                     weight_initializer_fn);

  if (bias_initializer_fn) {
    params->set_hasbias(true);
    init_weight_params(params->mutable_bias(), num_output_channels,
                       bias_initializer_fn);
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

void model_spec::add_channel_concat(const std::string& name,
                                    const std::vector<std::string>& inputs) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  for (const std::string& input : inputs) {
    layer->add_input(input);
  }
  layer->add_output(name);

  layer->mutable_concat();
}

void model_spec::add_softmax(const std::string& name,
                             const std::string& input) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  layer->mutable_softmax();
}

void model_spec::add_addition(const std::string& name,
                              const std::vector<std::string>& inputs) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  for (const std::string& input : inputs) {
    layer->add_input(input);
  }
  layer->add_output(name);

  layer->mutable_add();
}

void model_spec::add_multiplication(const std::string& name,
                                    const std::vector<std::string>& inputs) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  for (const std::string& input : inputs) {
    layer->add_input(input);
  }
  layer->add_output(name);

  layer->mutable_multiply();
}

void model_spec::add_exp(const std::string& name, const std::string& input) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  layer->mutable_unary()->set_type(
      CoreML::Specification::UnaryFunctionLayerParams::EXP);
}

void model_spec::add_scale(const std::string& name, const std::string& input,
                           const std::array<size_t, 3>& shape_c_h_w,
                           weight_initializer scale_initializer_fn) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  CoreML::Specification::ScaleLayerParams* params = layer->mutable_scale();
  size_t size = 1;
  for (size_t i = 0; i < 3; ++i) {
    params->add_shapescale(shape_c_h_w[i]);
    size *= shape_c_h_w[i];
  }

  init_weight_params(params->mutable_scale(), size, scale_initializer_fn);
}

void model_spec::add_constant(const std::string& name,
                              const std::array<size_t, 3>& shape_c_h_w,
                              weight_initializer weight_initializer_fn) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_output(name);

  CoreML::Specification::LoadConstantLayerParams* params =
      layer->mutable_loadconstant();
  size_t size = 1;
  for (size_t i = 0; i < 3; ++i) {
    params->add_shape(shape_c_h_w[i]);
    size *= shape_c_h_w[i];
  }

  init_weight_params(params->mutable_data(), size, weight_initializer_fn);
}

void model_spec::add_reshape(const std::string& name, const std::string& input,
                             const std::array<size_t, 4>& seq_c_h_w) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  CoreML::Specification::ReshapeLayerParams* params = layer->mutable_reshape();
  for (size_t i = 0; i < 4; ++i) {
    params->add_targetshape(seq_c_h_w[i]);
  }
}

void model_spec::add_permute(const std::string& name, const std::string& input,
                             const std::array<size_t, 4>& axis_permutation) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  CoreML::Specification::PermuteLayerParams* params = layer->mutable_permute();
  for (size_t i = 0; i < 4; ++i) {
    params->add_axis(axis_permutation[i]);
  }
}

void model_spec::add_channel_slice(
    const std::string& name, const std::string& input, int start_index,
    int end_index, size_t stride) {

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_output(name);

  CoreML::Specification::SliceLayerParams* params = layer->mutable_slice();
  params->set_startindex(start_index);
  params->set_endindex(end_index);
  params->set_stride(stride);
  params->set_axis(CoreML::Specification::SliceLayerParams::CHANNEL_AXIS);
}

void model_spec::add_lstm(
    const std::string& name, const std::string& input,
    const std::string& hidden_input, const std::string& cell_input,
    const std::string& hidden_output, const std::string& cell_output,
    size_t input_vector_size, size_t output_vector_size,
    float cell_clip_threshold, const lstm_weight_initializers& initializers) {

  const bool has_bias_vectors = initializers.input_gate_bias_fn
    && initializers.forget_gate_bias_fn
    && initializers.block_input_bias_fn
    && initializers.output_gate_bias_fn;

  NeuralNetworkLayer* layer = impl_->add_layers();
  layer->set_name(name);
  layer->add_input(input);
  layer->add_input(hidden_input);
  layer->add_input(cell_input);
  layer->add_output(name);
  layer->add_output(hidden_output);
  layer->add_output(cell_output);

  UniDirectionalLSTMLayerParams* params = layer->mutable_unidirectionallstm();
  params->set_inputvectorsize(input_vector_size);
  params->set_outputvectorsize(output_vector_size);
  params->add_activations()->mutable_sigmoid();
  params->add_activations()->mutable_tanh();
  params->add_activations()->mutable_tanh();

  CoreML::Specification::LSTMParams* lstm_params = params->mutable_params();
  lstm_params->set_hasbiasvectors(has_bias_vectors);
  lstm_params->set_cellclipthreshold(cell_clip_threshold);

  CoreML::Specification::LSTMWeightParams* lstm_weights =
      params->mutable_weightparams();
  init_weight_params(lstm_weights->mutable_inputgateweightmatrix(),
                     input_vector_size * output_vector_size,
                     initializers.input_gate_weight_fn);
  init_weight_params(lstm_weights->mutable_forgetgateweightmatrix(),
                     input_vector_size * output_vector_size,
                     initializers.forget_gate_weight_fn);
  init_weight_params(lstm_weights->mutable_blockinputweightmatrix(),
                     input_vector_size * output_vector_size,
                     initializers.block_input_weight_fn);
  init_weight_params(lstm_weights->mutable_outputgateweightmatrix(),
                     input_vector_size * output_vector_size,
                     initializers.output_gate_weight_fn);
  init_weight_params(lstm_weights->mutable_inputgaterecursionmatrix(),
                     output_vector_size * output_vector_size,
                     initializers.input_gate_recursion_fn);
  init_weight_params(lstm_weights->mutable_forgetgaterecursionmatrix(),
                     output_vector_size * output_vector_size,
                     initializers.forget_gate_recursion_fn);
  init_weight_params(lstm_weights->mutable_blockinputrecursionmatrix(),
                     output_vector_size * output_vector_size,
                     initializers.block_input_recursion_fn);
  init_weight_params(lstm_weights->mutable_outputgaterecursionmatrix(),
                     output_vector_size * output_vector_size,
                     initializers.output_gate_recursion_fn);
  init_weight_params(lstm_weights->mutable_inputgatebiasvector(),
                     output_vector_size, initializers.input_gate_bias_fn);
  init_weight_params(lstm_weights->mutable_forgetgatebiasvector(),
                     output_vector_size, initializers.forget_gate_bias_fn);
  init_weight_params(lstm_weights->mutable_blockinputbiasvector(),
                     output_vector_size, initializers.block_input_bias_fn);
  init_weight_params(lstm_weights->mutable_outputgatebiasvector(),
                     output_vector_size, initializers.output_gate_bias_fn);
}

}  // neural_net
}  // turi
