/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_COMPUTE_CONTEXT_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_COMPUTE_CONTEXT_HPP_

#include <memory>

#include <ml/neural_net/image_augmentation.hpp>
#include <ml/neural_net/model_backend.hpp>

namespace turi {
namespace neural_net {

/**
 * Interface for factories that produce concrete data augmentation and neural
 * network module instances, used to abstract across backend implementations and
 * hardware resources.
 */
class compute_context {
public:

  /**
   * Creates a compute context using a backend appropriate to the current
   * platform and hardware.
   */
  static std::unique_ptr<compute_context> create();

  virtual ~compute_context();

  /**
   * Returns the (human readable) names of the GPUs used by this context, for
   * reporting to the user.
   */
  virtual std::vector<std::string> gpu_names() const = 0;

  /**
   * Provides a measure of the memory resources available.
   *
   * Returns the maximum memory size in bytes that neural networks should
   * allocate, typically used to determine batch sizes (often heuristically).
   */
  virtual size_t memory_budget() const = 0;

  /**
   * Creates an object detection network.
   *
   * \todo Define a object_detector_config struct to encapsulate these
   *       parameters in a more self-documenting and typesafe way.
   * \todo Initialize the network directly from a model_spec, in lieu of passing
   *       weights as a float_array_map.
   */
  virtual std::unique_ptr<model_backend> create_object_detector(
      int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
      const float_array_map& config, const float_array_map& weights) = 0;

  /**
   * Creates an activity classification network.
   *
   * \todo Define an activity_classifier_config struct to encapsulate these
   *       parameters in a more self-documenting and typesafe way.
   * \todo Initialize the network directly from a model_spec, in lieu of passing
   *       weights as a float_array_map.
   */
  virtual std::unique_ptr<model_backend> create_activity_classifier(
      int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
      const float_array_map& config, const float_array_map& weights) = 0;

  /**
   * Creates an image augmenter.
   */
  virtual std::unique_ptr<image_augmenter> create_image_augmenter(
      const image_augmenter::options &opts) = 0;

};

}  // namespace neural_net
}  // namespace turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_COMPUTE_CONTEXT_HPP_
