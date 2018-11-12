/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_CNN_MODULE_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_CNN_MODULE_HPP_

#include <memory>

#include <unity/toolkits/neural_net/float_array.hpp>

namespace turi {
namespace neural_net {

/**
 * A pure virtual interface for neural networks, used to abstract across model
 * architectures and backend implementations.
 */
class cnn_module {
public:
  virtual ~cnn_module() = default;

  /**
   * Creates an object detection network using a backend appropriate to the
   * current platform and hardware.
   *
   * \todo Define a object_detector_config struct to encapsulate these
   *       parameters in a more self-documenting and typesafe way.
   * \todo Initialize the network directly from a model_spec, in lieu of passing
   *       weights as a float_array_map.
   */
  static std::unique_ptr<cnn_module> create_object_detector(
      int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
      const float_array_map& config, const float_array_map& weights);

  /**
   * Sets the learning rate to be used for future calls to train.
   */
  virtual void set_learning_rate(float lr) = 0;

  /**
   * Performs one forward-backward pass.
   */
  virtual deferred_float_array train(const float_array& input_batch,
                                     const float_array& label_batch) = 0;

  /**
   * Performs a forward pass.
   */
  virtual deferred_float_array
  predict(const float_array& input_batch) const = 0;

  /**
   * Exports the network weights.
   *
   * \todo Someday, once no Python frontend depends on this method, this could
   *       just take a mutable model_spec (updating the one used to initialize
   *       the cnn_module).
   */
  virtual float_array_map export_weights() const = 0;
};

}  // namespace neural_net
}  // namespace turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_CNN_MODULE_HPP_
