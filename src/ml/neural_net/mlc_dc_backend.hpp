/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#import <MLCompute/MLCompute.h>

#include <ml/neural_net/mlc_layer_weights.hpp>
#include <ml/neural_net/model_backend.hpp>

namespace turi {
namespace neural_net {

class API_AVAILABLE(macos(10.16)) mlc_drawing_classifier_backend : public model_backend {
 public:
  mlc_drawing_classifier_backend(MLCDevice *device, const float_array_map &weights,
                                 size_t batch_size, size_t num_classes);

  // model_backend interface
  float_array_map export_weights() const override;
  void set_learning_rate(float lr) override;
  float_array_map train(const float_array_map &inputs) override;
  float_array_map predict(const turi::neural_net::float_array_map &inputs) const override;

 private:
  MLCTrainingGraph *training_graph_ = nil;
  MLCInferenceGraph *inference_graph_ = nil;
  MLCTensor *input_ = nil;
  MLCTensor *weights_ = nil;
  MLCTensor *labels_ = nil;

  mlc_layer_weights layer_weights_;
  size_t num_classes_;
};

}  // namespace neural_net
}  // namespace turi
