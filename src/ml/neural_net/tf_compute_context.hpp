/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_TF_COMPUTE_CONTEXT_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_TF_COMPUTE_CONTEXT_HPP_

#include <core/util/try_finally.hpp>
#include <ml/neural_net/compute_context.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace turi {
namespace neural_net {

template <typename CallFunc>
void call_pybind_function(const CallFunc& func) {
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  turi::scoped_finally gstate_restore([&]() { PyGILState_Release(gstate); });

  try {
    func();
  } catch (...) {
    // TODO: Do better error logging
    log_and_throw("An error occurred!");
  }
}


/**
 * A compute_context implementation backed by TensorFlow
 * for neural network computation and for data
 * augmentation.
 */
class tf_compute_context: public compute_context {
public:


  /**
   * Constructs a context wrapping devices.
   */
  tf_compute_context();

  ~tf_compute_context();

  std::vector<std::string> gpu_names() const override;
  size_t memory_budget() const override;

  std::unique_ptr<model_backend> create_object_detector(
      int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
      const float_array_map& config, const float_array_map& weights) override;

  std::unique_ptr<model_backend> create_activity_classifier(
      int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
      const float_array_map& config, const float_array_map& weights) override;

  std::unique_ptr<image_augmenter> create_image_augmenter(
      const image_augmenter::options &opts) override;

  
  
};

class tf_model_backend : public model_backend {
 public:
  tf_model_backend(pybind11::object model);

  ~tf_model_backend();

  // model_backend interface
  float_array_map train(const float_array_map& inputs) override;
  float_array_map predict(const float_array_map& inputs) const override;
  float_array_map export_weights() const override;
  void set_learning_rate(float lr) override;

 private:
  pybind11::object model_;
};

}  // namespace neural_net
}  // namespace turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_TF_COMPUTE_CONTEXT_HPP_
