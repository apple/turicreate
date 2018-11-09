/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_MODEL_SPEC_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_MODEL_SPEC_HPP_

#include <functional>
#include <memory>
#include <string>

#include <unity/toolkits/neural_net/float_array.hpp>

// Forward declare CoreML::Specification::Model in lieu of including problematic
// protocol buffer headers.
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

  /**
   * Callback type used to initialize an underlying WeightParams instance.
   *
   * The callback should write the desired values into the provided iterator
   * range, which is initialized to 0.f.
   */
  using weight_initializer = std::function<void(float* first_weight,
                                                float* last_weight)>;

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
                       size_t kernel_size,
                       weight_initializer weight_initializer_fn,
                       weight_initializer bias_initializer_fn = nullptr);

  /**
   * Appends a batch norm layer.
   *
   * The beta and mean parameters are initialized to 0.f; the gamma and variance
   * parameters are initialized to 1.f
   *
   * \param nn_model The NeuralNetwork to which to append a batch norm layer
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param num_channels The C dimension of the input and output
   * \param epsilon Added to the variance for each input before normalizing
   */
  void add_batchnorm(const std::string& name, const std::string& input,
                     size_t num_channels, float epsilon);

  /**
   * Appends a leaky ReLU layer.
   *
   * \param nn_model The NeuralNetwork to which to append a leaky ReLU layer
   * \param name The name of the layer and its output
   * \param input The name of the layer's input
   * \param alpha Multiplied to negative inputs
   */
  void add_leakyrelu(const std::string& name, const std::string& input,
                     float alpha);

  // TODO: Support additional layers (and further parameterize the above) as
  // needed.

protected:

  /**
   * Exposes the underlying CoreML proto, mostly for testing.
   */
  const CoreML::Specification::NeuralNetwork& get_coreml_spec() const {
    return *impl_;
  }

private:

  std::unique_ptr<CoreML::Specification::NeuralNetwork> impl_;
};

}  // neural_net
}  // turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_MODEL_SPEC_HPP_
