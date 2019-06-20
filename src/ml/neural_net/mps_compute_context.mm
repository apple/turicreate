/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/mps_compute_context.hpp>

#import <ml/neural_net/mps_device_manager.h>

#include <core/logging/logger.hpp>
#include <ml/neural_net/mps_cnnmodule.h>
#include <ml/neural_net/mps_graph_cnnmodule.h>
#include <ml/neural_net/mps_image_augmentation.hpp>

namespace turi {
namespace neural_net {

mps_compute_context::mps_compute_context(
    std::unique_ptr<mps_command_queue> command_queue)
  : command_queue_(std::move(command_queue))
{}

mps_compute_context::mps_compute_context()
  : command_queue_(new mps_command_queue)
{
  @autoreleasepool {

  id <MTLDevice> dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
  if (dev == nil) {
    log_and_throw("No valid Metal device found.");
  }

  command_queue_->impl = [dev newCommandQueue];

  }  // @autoreleasepool
}

mps_compute_context::~mps_compute_context() = default;

size_t mps_compute_context::memory_budget() const {
  @autoreleasepool {

  id <MTLDevice> dev = command_queue_->impl.device;
  return static_cast<size_t>(dev.recommendedMaxWorkingSetSize);

  }  // @autoreleasepool
}

std::vector<std::string> mps_compute_context::gpu_names() const {
  @autoreleasepool {

  id <MTLDevice> dev = command_queue_->impl.device;
  return { [dev.name cStringUsingEncoding:NSUTF8StringEncoding] };

  }  // @autoreleasepool
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
      new mps_graph_cnn_module(*command_queue_));

  result->init(/* network_id */ kODGraphNet, n, c_in, h_in, w_in, c_out, h_out,
               w_out, config, weights);

  return result;
}

std::unique_ptr<model_backend> mps_compute_context::create_activity_classifier(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {

  std::unique_ptr<mps_cnn_module> result(new mps_cnn_module(*command_queue_));

  result->init(/* network_id */ kActivityClassifierNet, n, c_in, h_in, w_in,
               c_out, h_out, w_out, /* updater_id */ 2 /* Adam */,  config);
  result->load(weights);

  return result;
}

}  // namespace neural_net
}  // namespace turi
