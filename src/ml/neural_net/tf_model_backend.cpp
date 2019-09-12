/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/tf_model_backend.hpp>

#include <iostream>
#include <vector>

#include <core/util/try_finally.hpp>
#include <ml/neural_net/float_array.hpp>
#include <ml/neural_net/tf_compute_context.hpp>

#include <pybind11/eval.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace turi {
namespace neural_net {

using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::shared_float_array;

template <typename CallFunc>
auto call_pybind_function(const CallFunc&& func) -> decltype(func()) {
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

static std::vector<size_t> get_shape(const float_array& num) {
  return std::vector<size_t>(num.shape(), num.shape() + num.dim());
}

static std::vector<size_t> get_strides(const float_array& num) {
  std::vector<size_t> result(num.dim());
  const size_t* shape = num.shape();
  for (size_t i = num.dim() - 1; i < 0; i--) {
    if (i == num.dim() - 1) {
      result[i] = sizeof(float);
      std::cout << result[i];
    } else {
      result[i] = result[i + 1] * shape[i + 1];
    }
  }
  return result;
}

PYBIND11_MODULE(libtctensorflow, m) {
  pybind11::class_<float_array>(m, "FloatArray", pybind11::buffer_protocol())
      .def_buffer([](float_array& m) -> pybind11::buffer_info {
        return pybind11::buffer_info(
            const_cast<float*>(m.data()), /* Pointer to buffer */
            sizeof(float),                /* Size of one scalar */
            pybind11::format_descriptor<
                float>::format(), /* Python struct-style format descriptor */
            m.dim(),              /* Number of dimensions */
            get_shape(m),         /* Buffer dimensions */
            get_strides(m)        /* Strides (in bytes) for each index */

        );
      });
  pybind11::class_<shared_float_array>(m, "SharedFloatArray",
                                       pybind11::buffer_protocol())
      .def_buffer([](shared_float_array& m) -> pybind11::buffer_info {
        return pybind11::buffer_info(
            const_cast<float*>(m.data()), /* Pointer to buffer */
            sizeof(float),                /* Size of one scalar */
            pybind11::format_descriptor<
                float>::format(), /* Python struct-style format descriptor */
            m.dim(),              /* Number of dimensions */
            get_shape(m),         /* Buffer dimensions */
            get_strides(m)        /* Strides (in bytes) for each index */

        );
      });
}

tf_model_backend::tf_model_backend(pybind11::object model) { model_ = model; }

float_array_map tf_model_backend::train(const float_array_map& inputs) {
  // Call train method on ActivityTensorflowModel
  float_array_map result;

  call_pybind_function([&]() {
    pybind11::object output = model_.attr("train")(inputs);

    std::map<std::string, pybind11::buffer> buf_output =
        output.cast<std::map<std::string, pybind11::buffer>>();

    for (auto& kv : buf_output) {
      pybind11::buffer_info buf = kv.second.request();
      turi::neural_net::shared_float_array value =
          turi::neural_net::shared_float_array::copy(
              (float*)buf.ptr,
              std::vector<size_t>(buf.shape.begin(), buf.shape.end()));
      result[kv.first] = value;
    }
  });

  return result;
}

float_array_map tf_model_backend::predict(const float_array_map& inputs) const {
  float_array_map result;

  // Call predict method on ActivityTensorFlowModel
  call_pybind_function([&]() {
    pybind11::object output = model_.attr("predict")(inputs);
    std::map<std::string, pybind11::buffer> buf_output =
        output.cast<std::map<std::string, pybind11::buffer>>();

    for (auto& kv : buf_output) {
      pybind11::buffer_info buf = kv.second.request();
      turi::neural_net::shared_float_array value =
          turi::neural_net::shared_float_array::copy(
              (float*)buf.ptr,
              std::vector<size_t>(buf.shape.begin(), buf.shape.end()));
      result[kv.first] = value;
    }
  });

  return result;
}

float_array_map tf_model_backend::export_weights() const {
  float_array_map result;
  call_pybind_function([&]() {
    // Call export_weights method on ActivityTensorFLowModel
    pybind11::object exported_weights = model_.attr("export_weights")();
    std::map<std::string, pybind11::buffer> buf_output =
        exported_weights.cast<std::map<std::string, pybind11::buffer>>();

    for (auto& kv : buf_output) {
      pybind11::buffer_info buf = kv.second.request();
      turi::neural_net::shared_float_array value =
          turi::neural_net::shared_float_array::copy(
              (float*)buf.ptr,
              std::vector<size_t>(buf.shape.begin(), buf.shape.end()));
      result[kv.first] = value;
    }
  });

  return result;
}

void tf_model_backend::set_learning_rate(float lr) {
  float_array_map result;

  // Call set_learning_rate method on ActivityTensorFLowModel
  call_pybind_function([&]() { model_.attr("set_learning_rate")(lr); });
}

tf_model_backend::~tf_model_backend() {
  call_pybind_function([&]() { model_ = pybind11::object(); });
}

}  // namespace neural_net
}  // namespace turi
