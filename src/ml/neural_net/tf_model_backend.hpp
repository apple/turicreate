/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef UNITY_TOOLKITS_NEURAL_NET_TF_MODEL_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_TF_MODEL_HPP_

#include <ml/neural_net/model_backend.hpp>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

namespace turi {
namespace neural_net {

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

#endif  // UNITY_TOOLKITS_NEURAL_NET_TF_MODEL_HPP_
