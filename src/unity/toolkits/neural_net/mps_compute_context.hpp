/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_MPS_COMPUTE_CONTEXT_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_MPS_COMPUTE_CONTEXT_HPP_

#include <memory>

#include <unity/toolkits/neural_net/compute_context.hpp>

namespace turi {
namespace neural_net {

/**
 * A compute_context implementation backed by Apple frameworks: Metal
 * Performance Shaders for neural network computation and Core Image for data
 * augmentation.
 */
class mps_compute_context: public compute_context {
public:

  /**
   * Constructs a context wrapping the best currently available Metal device.
   *
   * \todo Guard against eGPU coming and going?
   */
  mps_compute_context();

  ~mps_compute_context();

  std::vector<std::string> gpu_names() const override;
  size_t memory_budget() const override;

  std::unique_ptr<model_backend> create_object_detector(
      int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
      const float_array_map& config, const float_array_map& weights) override;

  std::unique_ptr<model_backend> create_activity_classifier(
      int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
      const float_array_map& config, const float_array_map& weights) override;

  std::unique_ptr<image_augmenter> create_image_augmenter(
      const image_augmenter::options &opts) override;

  /**
   * Alternate implementation of create_image_augmenter supporting injection of
   * the random number generator, for test purposes.
   */
  std::unique_ptr<image_augmenter> create_image_augmenter_for_testing(
      const image_augmenter::options& opts,
      std::function<float(float lower, float upper)> rng);

private:

  // Use PIMPL pattern to keep Objective C out of this header.
  struct impl;

  std::unique_ptr<impl> impl_;
};

}  // namespace neural_net
}  // namespace turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_MPS_COMPUTE_CONTEXT_HPP_
