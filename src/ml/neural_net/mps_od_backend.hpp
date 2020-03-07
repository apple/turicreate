/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef MPS_OD_BACKEND_HPP_
#define MPS_OD_BACKEND_HPP_

#include <ml/neural_net/mps_graph_cnnmodule.h>
#include <ml/neural_net/model_backend.hpp>

namespace turi {
namespace neural_net {

/**
 * Model backend for object detection that uses a separate mps_graph_cnnmodule
 * for training and for inference, since mps_graph_cnnmodule doesn't currently
 * support doing both.
 */
class mps_od_backend : public model_backend {
 public:
  struct parameters {
    std::shared_ptr<mps_command_queue> command_queue;
    int n;
    int c_in;
    int h_in;
    int w_in;
    int c_out;
    int h_out;
    int w_out;
    float_array_map config;
    float_array_map weights;
  };

  mps_od_backend(parameters params);

  // Training
  void set_learning_rate(float lr) override;
  float_array_map train(const float_array_map& inputs) override;

  // Inference
  float_array_map predict(const float_array_map& inputs) const override;

  float_array_map export_weights() const override;

 private:
  void ensure_training_module();
  void ensure_prediction_module() const;

  parameters params_;

  std::unique_ptr<mps_graph_cnn_module> training_module_;

  // Cleared whenever the training module is updated.
  mutable std::unique_ptr<mps_graph_cnn_module> prediction_module_;
};

}  // namespace neural_net
}  // namespace turi

#endif  // MPS_OD_BACKEND_HPP_
