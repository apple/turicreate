/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#import <MLCompute/MLCompute.h>

#include <ml/neural_net/compute_context.hpp>

namespace turi {
namespace neural_net {

/**
 * A compute_context implementation backed by MLFoundation (and Core Image for
 * data augmentation).
 */
class API_AVAILABLE(macos(10.16)) mlc_compute_context : public compute_context {
 public:
  mlc_compute_context(MLCDevice* device);
  ~mlc_compute_context();

  void print_training_device_info() const override;
  size_t memory_budget() const override;

  std::unique_ptr<model_backend> create_object_detector(int n, int c_in, int h_in, int w_in,
                                                        int c_out, int h_out, int w_out,
                                                        const float_array_map& config,
                                                        const float_array_map& weights) override;

  std::unique_ptr<model_backend> create_activity_classifier(
      const ac_parameters& ac_params) override;

  std::unique_ptr<model_backend> create_drawing_classifier(const float_array_map& weights,
                                                           size_t batch_size,
                                                           size_t num_classes) override;

  std::unique_ptr<image_augmenter> create_image_augmenter(
      const image_augmenter::options& opts) override;

  std::unique_ptr<model_backend> create_style_transfer(const float_array_map& config,
                                                       const float_array_map& weights) override;

  std::unique_ptr<model_backend> create_multilayer_perceptron_classifier(
      int n, int c_in, int c_out, const std::vector<size_t>& layer_sizes,
      const turi::neural_net::float_array_map& config) override;

 protected:
  MLCDevice* GetDevice() const;
  
 private:
  MLCDevice* device_ = nil;
};

}  // namespace neural_net
}  // namespace turi
