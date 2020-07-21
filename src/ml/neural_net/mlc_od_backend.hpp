/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#import <MLCompute/MLCompute.h>

#include <ml/neural_net/model_backend.hpp>

#include <ml/neural_net/mlc_layer_weights.hpp>

// Forward declarations necessary to avoid header-inclusion issues. See the
// .cpp file for more details.
@class TCModelTrainerBackendGraphs;
namespace turi {
namespace object_detection {
class DarknetYOLOCheckpoint;
}
}  // namespace turi

namespace turi {
namespace neural_net {

class API_AVAILABLE(macos(10.16)) mlc_object_detector_backend : public model_backend {
 public:
  // Expose the construction of the underlying MLFoundation graphs, for testing
  // purposes.
  static TCModelTrainerBackendGraphs *create_graphs(
      const turi::object_detection::DarknetYOLOCheckpoint &checkpoint);

  // TODO: No, really, replace this legacy interface with one that just accepts
  // a struct with explicit, named parameters for OD specifically.
  mlc_object_detector_backend(MLCDevice *device, size_t n, size_t c_in, size_t h_in, size_t w_in,
                              size_t c_out, size_t h_out, size_t w_out,
                              const float_array_map &config, const float_array_map &weights);

  // model_backend interface
  float_array_map export_weights() const override;
  void set_learning_rate(float lr) override;
  float_array_map train(const float_array_map &inputs) override;
  float_array_map predict(const float_array_map &inputs) const override;

 private:
  static TCModelTrainerBackendGraphs *create_graphs(size_t n, size_t c_in, size_t h_in, size_t w_in,
                                                    size_t c_out, size_t h_out, size_t w_out,
                                                    const float_array_map &config,
                                                    const float_array_map &weights,
                                                    mlc_layer_weights *layer_weights);

  // TODO: Replace these with an instance of TCModelTrainerBackendGraphs?
  MLCTrainingGraph *training_graph_ = nil;
  MLCInferenceGraph *inference_graph_ = nil;
  MLCTensor *input_ = nil;
  MLCTensor *labels_ = nil;

  mlc_layer_weights layer_weights_;
  std::vector<size_t> output_shape_;
};

}  // namespace neural_net
}  // namespace turi
