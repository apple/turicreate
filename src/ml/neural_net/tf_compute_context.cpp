/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
*/

#include <iostream>
#include <vector>
#include <ml/neural_net/float_array.hpp>
#include <ml/neural_net/tf_compute_context.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
namespace py = pybind11;

py::array_t<float> import_pyobj(){

  py::module calc = py::module::import("pymod.do_stuff");
  py::object result = calc.attr("NumpyObject")();
  py::object res = result.attr("add")();
  py::buffer n = res.cast<py::buffer>();
  py::array_t<float> num = res.cast<py::array_t<float>>();
  py::buffer_info buf = n.request();
  
  turi::neural_net::shared_float_array shrpy = turi::neural_net::shared_float_array::copy((float *)buf.ptr, std::vector<size_t>(buf.shape.begin(),buf.shape.end()));
  std::cout << shrpy;
  return num;
 }




PYBIND11_MODULE(libtctf, m) {
  m.def("import_pyobj", &import_pyobj, "Add two numpy arrays");
}

namespace turi {
namespace neural_net {

namespace {

std::unique_ptr<compute_context> create_tf_compute_context() {
  return std::unique_ptr<compute_context>(new tf_compute_context);
}

// At static-init time, register create_mxnet_compute_context().
// TODO: Codify priority levels?
static auto* tf_registration = new compute_context::registration(
    /* priority */ 1, &create_tf_compute_context);

}  // namespace

tf_compute_context::tf_compute_context()
{}


tf_compute_context::~tf_compute_context() = default;

size_t tf_compute_context::memory_budget() const {
  return 0;
}

std::vector<std::string> tf_compute_context::gpu_names() const {
  return std::vector<std::string>();
}

std::unique_ptr<image_augmenter> tf_compute_context::create_image_augmenter(
    const image_augmenter::options& opts) {
  return std::unique_ptr<image_augmenter>();
}


std::unique_ptr<model_backend> tf_compute_context::create_object_detector(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {
  return std::unique_ptr<model_backend>();
}

std::unique_ptr<model_backend> tf_compute_context::create_activity_classifier(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {
  return std::unique_ptr<model_backend>();
}

}  // namespace neural_net
}  // namespace turi
