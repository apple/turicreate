/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/mps_od_backend.hpp>

namespace turi {
namespace neural_net {

void mps_od_backend::ensure_training_module() {
  if (training_module_) return;

  training_module_.reset(new mps_graph_cnn_module(*params_.command_queue));
  training_module_->init(/* network_id */ kODGraphNet, params_.n, params_.c_in, params_.h_in,
                         params_.w_in, params_.c_out, params_.h_out, params_.w_out, params_.config,
                         params_.weights);

  // Clear params_.weights to free up memory, since they are now superceded by
  // whatever the training module contains.
  params_.weights.clear();
}

void mps_od_backend::ensure_prediction_module() const {
  if (prediction_module_) return;

  // Adjust configuration for prediction.
  float_array_map config = params_.config;
  config["mode"] = shared_float_array::wrap(2.0f);
  config["od_include_loss"] = shared_float_array::wrap(0.0f);

  // Take weights from training module if present, else from original weights.
  float_array_map weights;
  if (training_module_) {
    weights = training_module_->export_weights();
  } else {
    weights = params_.weights;
  }

  prediction_module_.reset(new mps_graph_cnn_module(*params_.command_queue));
  prediction_module_->init(/* network_id */ kODGraphNet, params_.n, params_.c_in, params_.h_in,
                           params_.w_in, params_.c_out, params_.h_out, params_.w_out, config,
                           weights);
}

mps_od_backend::mps_od_backend(parameters params) : params_(std::move(params)) {
  // Immediate instantiate at least one module, since at present we can't
  // guarantee that the weights will remain valid after we return.
  // TODO: Remove this eager construction once we stop putting weak pointers in
  // float_array_map.
  if (params_.config.at("mode").data()[0] == 0.f) {
    ensure_training_module();
  } else {
    ensure_prediction_module();
  }
}

void mps_od_backend::set_learning_rate(float lr) {
  ensure_training_module();
  training_module_->set_learning_rate(lr);
}

float_array_map mps_od_backend::train(const float_array_map& inputs) {
  // Invalidate prediction_module, since its weights will be stale.
  prediction_module_.reset();

  ensure_training_module();
  return training_module_->train(inputs);
}

float_array_map mps_od_backend::predict(const float_array_map& inputs) const {
  ensure_prediction_module();
  return prediction_module_->predict(inputs);
}

float_array_map mps_od_backend::export_weights() const {
  if (training_module_) {
    return training_module_->export_weights();
  } else {
    return params_.weights;
  }
}

}  // namespace neural_net
}  // namespace turi
