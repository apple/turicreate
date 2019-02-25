/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/neural_net/mps_compute_context.hpp>

#import <unity/toolkits/neural_net/mps_device_manager.h>

#include <logger/logger.hpp>
#include <unity/toolkits/neural_net/mps_cnnmodule.h>
#include <unity/toolkits/neural_net/mps_graph_cnnmodule.h>
#include <unity/toolkits/neural_net/mps_image_augmentation.hpp>

namespace turi {
namespace neural_net {

struct mps_compute_context::impl {

  id <MTLDevice> dev = nil;
};

mps_compute_context::mps_compute_context(): impl_(new impl) {

  impl_->dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
  if (impl_->dev == nil) {
    log_and_throw("No valid Metal device found.");
  }
}

mps_compute_context::~mps_compute_context() = default;

size_t mps_compute_context::memory_budget() const {

  return static_cast<size_t>(impl_->dev.recommendedMaxWorkingSetSize);
}

std::vector<std::string> mps_compute_context::gpu_names() const {
  return { [impl_->dev.name cStringUsingEncoding:NSUTF8StringEncoding] };
}

std::unique_ptr<image_augmenter> mps_compute_context::create_image_augmenter(
    const image_augmenter::options& opts) {

  return std::unique_ptr<image_augmenter>(new mps_image_augmenter(opts));
}

std::unique_ptr<image_augmenter>
mps_compute_context::create_image_augmenter_for_testing(
    const image_augmenter::options& opts,
    std::function<float(float lower, float upper)> rng) {

  return std::unique_ptr<image_augmenter>(new mps_image_augmenter(opts, rng));
}

std::unique_ptr<model_backend> mps_compute_context::create_object_detector(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {

  std::unique_ptr<mps_graph_cnn_module> result(
      new mps_graph_cnn_module(impl_->dev));

  result->init(/* network_id */ kODGraphNet, n, c_in, h_in, w_in, c_out, h_out,
               w_out, config, weights);

  return result;
}

std::unique_ptr<model_backend> mps_compute_context::create_activity_classifier(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {

  std::unique_ptr<mps_cnn_module> result(new mps_cnn_module(impl_->dev));

  result->init(/* network_id */ kActivityClassifierNet, n, c_in, h_in, w_in,
               c_out, h_out, w_out, /* updater_id */ 2 /* Adam */,  config);
  result->load(weights);

  return result;
}

}  // namespace neural_net
}  // namespace turi
