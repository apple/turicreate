/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/mps_compute_context.hpp>

#import <ml/neural_net/mps_device_manager.h>

#include <core/logging/logger.hpp>
#include <core/storage/fileio/fileio_constants.hpp>
#include <ml/neural_net/mps_cnnmodule.h>
#include <ml/neural_net/mps_graph_cnnmodule.h>
#include <ml/neural_net/mps_image_augmentation.hpp>

#import <ml/neural_net/style_transfer/mps_style_transfer_backend.hpp>

namespace turi {
namespace neural_net {

namespace {

float_array_map multiply_mps_od_loss_multiplier(float_array_map config,
                                                const std::vector<std::string>& update_keys) {
  for (const std::string& key : update_keys) {
    auto config_iter = config.find(key);
    if (config_iter != config.end()) {
      float old_value = config_iter->second.data()[0];
      float new_value;
      if (key == "learning_rate") {
        new_value = old_value / MPS_OD_LOSS_MULTIPLIER;
      } else {
        new_value = old_value * MPS_OD_LOSS_MULTIPLIER;
      }
      config_iter->second = shared_float_array::wrap(new_value);
    }
  }
  return config;
}

std::unique_ptr<compute_context> create_mps_compute_context() {
  @autoreleasepool {

  // If the user has disabled GPU usage, don't use MPS at all.
  if (fileio::NUM_GPUS == 0) return nullptr;

  std::unique_ptr<compute_context> result;

  // Query the best available Metal device.
  // \todo Guard against eGPU coming and going?
  id<MTLDevice> dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];

  // Discrete GPUs should work, but the story for integrated "low-power" GPUs is more complex.
  bool supported = true;
  if (dev.lowPower) {
    if (@available(macOS 10.15, *)) {
      // Intel HD 515 or later is supported.
      supported = [dev supportsFeatureSet:MTLFeatureSet_macOS_GPUFamily2_v1];
    } else {
      // For our use cases, older versions of macOS cannot robustly support integrated GPUs.
      supported = false;
    }
  }

  // If we have a supported Metal device, wrap a mps_command_queue around it.
  if (supported) {
    std::unique_ptr<mps_command_queue> command_queue(new mps_command_queue);
    command_queue->impl = [dev newCommandQueue];
    result.reset(new mps_compute_context(std::move(command_queue)));
  }

  return result;

  }  // @autoreleasepool
}

// At static-init time, register create_mps_compute_context().
// TODO: Codify priority levels?
static auto* mps_registration = new compute_context::registration(
    /* priority */ 0, &create_mps_compute_context, nullptr);

}  // namespace

mps_compute_context::mps_compute_context(
    std::unique_ptr<mps_command_queue> command_queue)
  : command_queue_(std::move(command_queue))
{}

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

// static
std::unique_ptr<image_augmenter>
mps_compute_context::create_image_augmenter_for_testing(
    const image_augmenter::options& opts,
    std::function<float(float lower, float upper)> rng) {

  return std::unique_ptr<image_augmenter>(new mps_image_augmenter(opts, rng));
}

std::unique_ptr<model_backend> mps_compute_context::create_object_detector(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {
  float_array_map updated_config;
  std::vector<std::string> update_keys = {
      "learning_rate", "od_scale_class", "od_scale_no_object", "od_scale_object",
      "od_scale_wh",   "od_scale_xy",    "gradient_clipping"};
  updated_config = multiply_mps_od_loss_multiplier(config, update_keys);
  std::unique_ptr<mps_graph_cnn_module> result(
      new mps_graph_cnn_module(*command_queue_));

  result->init(/* network_id */ kODGraphNet, n, c_in, h_in, w_in, c_out, h_out,
               w_out, updated_config, weights);

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

std::unique_ptr<model_backend> mps_compute_context::create_style_transfer(
    const float_array_map& config, const float_array_map& weights) {
#ifdef HAS_MACOS_10_15
  return std::unique_ptr<model_backend>(
      new style_transfer::mps_style_transfer(config, weights, *command_queue_));
#else
  return nullptr;
#endif
}

std::unique_ptr<model_backend> mps_compute_context::create_drawing_classifier(
    /* TODO: const float_array_map& config if needed */
    const float_array_map& weights,
    size_t batch_size, size_t num_classes) {
  return nullptr;
}

}  // namespace neural_net
}  // namespace turi
