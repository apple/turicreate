/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
*/
#ifndef UNITY_TOOLKITS_NEURAL_NET_TF_AC_MODEL_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_TF_AC_MODEL_HPP_

#include <ml/neural_net/model_backend.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace turi {
namespace neural_net {

namespace py = pybind11;

class tf_activity_classifier_backend: public model_backend {
public:

  tf_activity_classifier_backend(int batch_size, int num_features, 
	int num_classes, int predictions_in_chunk, const float_array_map& net_params, const float_array_map& config);

  ~tf_activity_classifier_backend();

  // model_backend interface
  float_array_map train(const float_array_map& inputs) override;
  float_array_map predict(const float_array_map& inputs) const override;
  float_array_map export_weights() const override;
  void set_learning_rate(float lr) override;



private:
	py::object activity_classifier_;
};
} //neural_net
} // turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_TF_AC_MODEL_HPP_