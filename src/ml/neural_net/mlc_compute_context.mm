/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/neural_net/mlc_compute_context.hpp>

#import <Metal/Metal.h>
#import <ml/neural_net/mps_device_manager.h>

#include <numeric>

#include <core/logging/logger.hpp>
#include <core/util/std/make_unique.hpp>

#include <ml/neural_net/mlc_dc_backend.hpp>
#include <ml/neural_net/mlc_od_backend.hpp>
#include <ml/neural_net/mps_compute_context.hpp>
#include <ml/neural_net/mps_image_augmentation.hpp>

namespace turi {
namespace neural_net {

namespace {

std::unique_ptr<compute_context> create_mlc_compute_context()
{
  if (@available(macOS 10.16, *)) {
    MLCDevice* device = [MLCDevice deviceWithType:MLCDeviceTypeAny];
    return std::unique_ptr<compute_context>(new mlc_compute_context(device));
  }
  return nullptr;
}

// At static-init time, register create_mps_compute_context().
// TODO: Codify priority levels
static auto* mlc_registration = new compute_context::registration(
    /* priority */ 0, nullptr, &create_mlc_compute_context, &create_mlc_compute_context);
}

mlc_compute_context::mlc_compute_context(MLCDevice* device)
  : device_(device)
{
}

mlc_compute_context::~mlc_compute_context() = default;

size_t mlc_compute_context::memory_budget() const
{
  size_t result = 0;
  if (device_.type == MLCDeviceTypeCPU) {
    // For now, report a memory budget that yields the Object Detector batch
    // size that has actually been test.
    // TODO: Clean up logic surrounding batch size and memory budget.
    result = 4294967296lu;  // "4GB"
  } else {
    // TODO: When MLFoundation supports training across multiple GPUs, ensure
    // this yields the right behavior with regard to e.g. choosing batch size.
    for (id<MTLDevice> dev in device_.gpuDevices) {
      result += static_cast<size_t>(dev.recommendedMaxWorkingSetSize);
    }
  }
  return result;
}

void mlc_compute_context::print_training_device_info() const
{
  std::vector<std::string> gpu_names;
  if (device_.type != MLCDeviceTypeCPU) {
    for (id<MTLDevice> dev in device_.gpuDevices) {
      const char* name = [dev.name cStringUsingEncoding:NSUTF8StringEncoding];
      if (name) {
        gpu_names.push_back(name);
      }
    }
  }
  if (gpu_names.empty()) {
    logprogress_stream << "Using CPU to create model";
  } else {
    std::string gpu_names_string = gpu_names[0];
    for (size_t i = 1; i < gpu_names.size(); ++i) {
      gpu_names_string += ", " + gpu_names[i];
    }
    logprogress_stream << "Using " << (gpu_names.size() > 1 ? "GPUs" : "GPU")
                       << " to create model (" << gpu_names_string << ")";
  }
}

std::unique_ptr<image_augmenter> mlc_compute_context::create_image_augmenter(
    const image_augmenter::options& opts)
{
  // This doesn't actually use MPS, it's just the CoreImage-based image
  // augmenter originally designed for the MPS compute context.
  return std::make_unique<mps_image_augmenter>(opts);
}

std::unique_ptr<model_backend> mlc_compute_context::create_object_detector(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights)
{
  if (@available(macOS 10.16, *)) {
    return std::make_unique<mlc_object_detector_backend>(device_, n, c_in, h_in, w_in, c_out, h_out,
                                                         w_out, config, weights);
  }
  return nullptr;
}

std::unique_ptr<model_backend> mlc_compute_context::create_activity_classifier(
    const ac_parameters& ac_params)
{
  return nullptr;
}

std::unique_ptr<model_backend> mlc_compute_context::create_drawing_classifier(
    const float_array_map& weights, size_t batch_size, size_t num_classes)
{
  if (@available(macOS 10.16, *)) {
    return std::make_unique<mlc_drawing_classifier_backend>(device_, weights, batch_size,
                                                            num_classes);
  }
  return nullptr;
}

std::unique_ptr<model_backend> mlc_compute_context::create_style_transfer(
    const float_array_map& config, const float_array_map& weights)
{
  return nullptr;
}

std::unique_ptr<model_backend> mlc_compute_context::create_multilayer_perceptron_classifier(
    int n, int c_in, int c_out, const std::vector<size_t>& layer_sizes,
    const float_array_map& config)
{
  return nullptr;
}

MLCDevice* mlc_compute_context::GetDevice() const { return device_; }

}  // namespace neural_net
}  // namespace turi
