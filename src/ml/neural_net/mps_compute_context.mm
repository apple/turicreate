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

#import <ml/neural_net/style_transfer/mps_style_transfer_backend.hpp>

namespace turi {
namespace neural_net {

namespace {

std::unique_ptr<compute_context> create_mps_compute_context() {
  return std::unique_ptr<compute_context>(new mps_compute_context);
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
<<<<<<< HEAD
=======

>>>>>>> move mps_loss_multiplier
  float_array_map updated_config;
  constexpr float MPS_LOSS_MULTIPLIER = 8;
  if (config.count("learning_rate") > 0){
    std::cout << 1;
    updated_config["learning_rate"] =
      shared_float_array::wrap(*config.at("learning_rate").data() / MPS_LOSS_MULTIPLIER);
  }
  if (config.count("od_scale_class") > 0){
  updated_config["od_scale_class"] =
      shared_float_array::wrap(*config.at("od_scale_class").data() * MPS_LOSS_MULTIPLIER);
  }
  if (config.count("od_scale_no_object") > 0){
  updated_config["od_scale_no_object"] =
      shared_float_array::wrap(*config.at("od_scale_no_object").data() * MPS_LOSS_MULTIPLIER);
  }
  if (config.count("od_scale_object") > 0){
  updated_config["od_scale_object"] =
      shared_float_array::wrap(*config.at("od_scale_object").data() * MPS_LOSS_MULTIPLIER);
  }
  if (config.count("od_scale_wh") > 0){
  updated_config["od_scale_wh"] =
      shared_float_array::wrap(*config.at("od_scale_wh").data() * MPS_LOSS_MULTIPLIER);
  }
  if (config.count("od_scale_xy") > 0){
  updated_config["od_scale_xy"] =
      shared_float_array::wrap(*config.at("od_scale_xy").data() * MPS_LOSS_MULTIPLIER);
  }
  updated_config["gradient_clipping"] =
      shared_float_array::wrap(*config.at("gradient_clipping").data() * MPS_LOSS_MULTIPLIER);
  
  for (const auto& kv : config) {
    if (updated_config.count(kv.first) == 0 ){
      updated_config[kv.first] = kv.second;
    }
  }

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
