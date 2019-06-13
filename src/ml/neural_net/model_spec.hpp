/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_MODEL_SPEC_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_MODEL_SPEC_HPP_

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <unity/toolkits/neural_net/float_array.hpp>
#include <unity/toolkits/neural_net/weight_init.hpp>

// Forward declare CoreML::Specification::NeuralNetwork in lieu of including
// problematic protocol buffer headers.
namespace CoreML {
namespace Specification {
class NeuralNetwork;
}
}

namespace turi {
namespace neural_net {

/**
 * Representation for a neural-network model (structure and parameters),
 * optimized for convenient export to CoreML.
 *
 * This class just wraps CoreML::Specification::NeuralNetwork, helping to
 * insulate toolkits from protobuf code.
 */
class model_spec {
public:

  /** Parameter for convolution and pooling layers. */
  enum class padding_type {
    VALID,
    SAME,
  };

  /**
   * Creates an empty model_spec (with no layers).
   */
  model_spec();

  /**
   * Initializes a model_spec from a NeuralNetwork proto.
   */
  model_spec(const CoreML::Specification::NeuralNetwork& nn_model);

  /**
   * Initializes a model_spec from the top-level NeuralNetwork found inside a
   * CoreML model specification on disk.
   *
   * \param mlmodel_path Path to a CoreM::Specification::Model proto on disk.
   * \throw If the indicated path could not be read or parsed.
   */
  model_spec(const std::string& mlmodel_path);

  // Declared here and defined in the .cpp file just to prevent the implicit
  // default destructor from attempting (and failing) to instantiate
  // std::unique_ptr<NeuralNetwork>::~unique_ptr()
  ~model_spec();

  /**
   * Exposes the underlying CoreML proto.
   */
  const CoreML::Specification::NeuralNetwork& get_coreml_spec() const {
    return *impl_;
  }

  /**
   * Transfer ownership of the underlying CoreML proto, invalidating the current
   * instance (leaving it in a "moved-from" state).
   *
   * (Note that this method may only be invoked from a model_spec&&)
   */
  std::unique_ptr<CoreML::Specification::NeuralNetwork> move_coreml_spec() &&;

  /**
   * Creates a shared_float_array view (weak reference) into the parameters of
   * the model, indexed by layer name.
   *
   * \return A dictionary whose keys are of the form
   *         "$layername_$paramname". The layer names are taken from the name
   *         field of each NeuralNetworkLayer containing a supported layer. The
   *         supported layers are ConvolutionLayerParams (with params "weight"
   *         (in NCHW order) and "bias") and BatchnormLayerParams (with params
   *         "gamma", "beta", "running_mean", and "running_var").
   * \throw If a NeuralNetworkLayer in the specification seems malformed
   *        (e.g. WeightParams with size inconsistent with declared layer
   *        shape).
   *
   * To avoid copying data, the data backing the shared_float_array instances in
   * the return value will only remain valid for the lifetime of this instance!
   */
  float_array_map export_params_view() const;

  /**
   * Overwrites existing WeightParams values using the provided float_array
   * values.
   *
   * \param weights A dictionary whose keys follow the same naming scheme used
   *                by `export_params_view`.
   * \throw If a float_array's shape does not match the corresponding
   *        NeuralNetworkLayer.
   */
  void update_params(const float_array_map& weights);

  /**
   * Determines whether the neural network contains a layer with the given
   * output name.
   *
   * In general, it is only safe to add a new layer that takes a named input if
   * this method returns true for that name.
   */
  bool has_layer_output(const std::string& layer_name) const;

  /**
   * Appends a ReLU activation layer.
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   */
  void add_relu(const std::string& name, const std::string& input);

  /**
   * Appends a leaky ReLU activation layer.
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param alpha Multiplied to negative inputs
   */
  void add_leakyrelu(const std::string& name, const std::string& input,
                     float alpha);

  /**
   * Appends a sigmoid activation layer.
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   */
  void add_sigmoid(const std::string& name, const std::string& input);

  /**
   * Appends a convolution layer.
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param num_output_channels The number of distinct filters in this layer
   * \param num_kernel_channels The number of input features per "pixel"
   * \param kernel_size The height and width of the kernel
   * \param weight_initializer_fn Callback used to initialize the conv weights
   * \param bias_initializer_fn Callback used to initialize the conv bias. If
   *                            nullptr, then no bias vector is set.
   */
  void add_convolution(const std::string& name, const std::string& input,
                       size_t num_output_channels, size_t num_kernel_channels,
                       size_t kernel_height, size_t kernel_width,
                       size_t stride_h, size_t stride_w, padding_type padding,
                       weight_initializer weight_initializer_fn,
                       weight_initializer bias_initializer_fn = nullptr);

  /**
   * Appends an inner-product (dense, fully connected) layer.
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param num_output_channels Size of the output vector
   * \param num_input_channels Size of the input vector
   * \param weight_initializer_fn Callback used to initialize the weights
   * \param bias_initializer_fn Callback used to initialize the bias. If
   *            nullptr, then no bias vector is set.
   */
  void add_inner_product(const std::string& name, const std::string& input,
                         size_t num_output_channels, size_t num_input_channels,
                         weight_initializer weight_initializer_fn,
                         weight_initializer bias_initializer_fn = nullptr);

  /**
   * Appends a batch norm layer.
   *
   * The beta and mean parameters are initialized to 0.f; the gamma and variance
   * parameters are initialized to 1.f
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param num_channels The C dimension of the input and output
   * \param epsilon Added to the variance for each input before normalizing
   */
  void add_batchnorm(const std::string& name, const std::string& input,
                     size_t num_channels, float epsilon);

  /**
   * Appends a layer that concatenates its inputs along the channel axis.
   *
   * \param name The name of the layer and its output
   * \param inputs The names of the layer's inputs
   */
  void add_channel_concat(const std::string& name,
                          const std::vector<std::string>& inputs);

  /**
   * Appends a layer that performs softmax normalization (along channel axis).
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   */
  void add_softmax(const std::string& name, const std::string& input);

  /**
   * Appends a layer that performs elementwise addition.
   *
   * \param name The name of the layer and its output
   * \param inputs The names of the layer's inputs
   */
  void add_addition(const std::string& name,
                    const std::vector<std::string>& inputs);

  /**
   * Appends a layer that performs elementwise multiplication.
   *
   * \param name The name of the layer and its output
   * \param inputs The names of the layer's inputs
   */
  void add_multiplication(const std::string& name,
                          const std::vector<std::string>& inputs);

  /**
   * Appends a layer that applies the unary function f(x) = e^x to its input.
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   */
  void add_exp(const std::string& name, const std::string& input);


  /**
   * Appends a layer that performs elementwise multiplication between its input
   * and some fixed weights.
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param shape_c_h_w The shape of the input and output
   * \param weight_initializer_fn Callback used to initialize the weights
   */
  void add_scale(const std::string& name, const std::string& input,
                 const std::array<size_t, 3>& shape_c_h_w,
                 weight_initializer scale_initializer_fn);

  /**
   * Appends a layer with fixed values.
   *
   * \param name The name of the layer and its output
   * \param shape_c_h_w The shape of the output
   * \param weight_initializer_fn Callback used to initialize the weights
   */
  void add_constant(const std::string& name,
                    const std::array<size_t, 3>& shape_c_h_w,
                    weight_initializer weight_initializer_fn);

  /**
   * Appends a layer that reshapes its input.
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param shape_c_h_w The shape of the output
   */
  void add_reshape(const std::string& name, const std::string& input,
                   const std::array<size_t, 4>& seq_c_h_w);

  /**
   * Appends a layer that transposes the dimensions of its input
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param axis_permutation A permutation of [0, 1, 2, 3], describing how to
   *            rearrange the [Seq, C, H, W] input.
   */
  void add_permute(const std::string& name, const std::string& input,
                   const std::array<size_t, 4>& axis_permutation);

  /**
   * Appends a layer that slices the input along the channel axis.
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param start_index The first channel to include
   * \param end_index The first channel to stop including. If negative, then the
   *            number of channels is added first (so -1 becomes n - 1).
   * \param stride The interval between channels to include
   */
  void add_channel_slice(const std::string& name, const std::string& input,
                         int start_index, int end_index, size_t stride);

  /**
   * Appends an LSTM layer.
   *
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param hidden_input The name of the initial hidden state
   * \param cell_input The name of the initial cell state
   * \param hidden_output The name of the resulting hidden state
   * \param cell_output The name of the resulting cell state
   * \param input_vector_size The size of the input vector
   * \param output_vector_size The size of the output vector (hidden state and
   *            cell state)
   * \param cell_clip_threshold Maximum magnitude of cell state values
   * \param initializers LSTM weights
   */
  void add_lstm(const std::string& name, const std::string& input,
                const std::string& hidden_input, const std::string& cell_input,
                const std::string& hidden_output,
                const std::string& cell_output, size_t input_vector_size,
                size_t output_vector_size, float cell_clip_threshold,
                const lstm_weight_initializers& initializers);

  // TODO: Support additional layers (and further parameterize the above) as
  // needed. If/when we support the full range of NeuralNetworkLayer values,
  // this could be shared in some form with coremltools.

private:

  std::unique_ptr<CoreML::Specification::NeuralNetwork> impl_;
};

}  // neural_net
}  // turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_MODEL_SPEC_HPP_
