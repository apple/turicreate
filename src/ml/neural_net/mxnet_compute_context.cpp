/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
*/

#include <iostream>
#include <vector>

#include <ml/neural_net/mxnet_compute_context.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace turi {
namespace neural_net {
namespace py = pybind11;
namespace {

std::unique_ptr<compute_context> create_mxnet_compute_context() {
  return std::unique_ptr<compute_context>(new mxnet_compute_context);
}

// At static-init time, register create_mxnet_compute_context().
// TODO: Codify priority levels?
static auto* mxnet_registration = new compute_context::registration(
    /* priority */ 1, &create_mxnet_compute_context);

}  // namespace

mxnet_compute_context::mxnet_compute_context()
{}


mxnet_compute_context::~mxnet_compute_context() = default;

size_t mxnet_compute_context::memory_budget() const {
  return 0;
}

std::vector<std::string> mxnet_compute_context::gpu_names() const {
  return std::vector<std::string>();
}

std::unique_ptr<image_augmenter> mxnet_compute_context::create_image_augmenter(
    const image_augmenter::options& opts) {
  return std::unique_ptr<image_augmenter>();
}


std::unique_ptr<model_backend> mxnet_compute_context::create_object_detector(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {
  return std::unique_ptr<model_backend>();
}

std::unique_ptr<model_backend> mxnet_compute_context::create_activity_classifier(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {
  return std::unique_ptr<model_backend>();
}

}  // namespace neural_net
}  // namespace turi
