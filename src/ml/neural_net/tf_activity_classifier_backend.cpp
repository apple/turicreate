/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
*/

#include <ml/neural_net/tf_activity_classifier_backend.hpp>

#include <iostream>
#include <vector>

#include <ml/neural_net/float_array.hpp>
#include <ml/neural_net/tf_compute_context.hpp>
#include <core/util/try_finally.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/eval.h>

namespace turi {
namespace neural_net {

namespace py = pybind11;

using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::shared_float_array;


template <typename CallFunc> 
auto call_pybind_function(CallFunc&& func) -> decltype(func()) {
    PyGILState_STATE gstate;
  	gstate = PyGILState_Ensure();

    turi::scoped_finally gstate_restore([&](){PyGILState_Release(gstate);});

	try {
		func();
	} catch(...){
	    	log_and_throw("An error occurred!");
	    }
}


static std::vector<size_t> get_dimensions( const float_array& num) {
  std::vector<size_t> result(num.dim());
  std::memcpy(result.data(), num.shape(), num.dim()* sizeof(size_t));
  return result;
}

static std::vector<size_t> get_strides( const float_array& num) {
  std::vector<size_t> result(num.dim());
  std::fill(result.begin(), result.end(), sizeof(float));
  return result;
}

PYBIND11_MODULE(libtctensorflow, m) {
	py::class_<float_array>(m, "FloatArray", py::buffer_protocol())
	.def_buffer([](float_array &m) -> py::buffer_info{
	      return py::buffer_info(
	          const_cast<float*>(m.data()),                   /* Pointer to buffer */
	          sizeof(float),                                  /* Size of one scalar */
	          py::format_descriptor<float>::format(),         /* Python struct-style format descriptor */
	          m.dim(),                                        /* Number of dimensions */
	          get_dimensions(m),                              /* Buffer dimensions */
	          get_strides(m)                                  /* Strides (in bytes) for each index */
	           
	      );
	});
	py::class_<shared_float_array>(m, "SharedFloatArray", py::buffer_protocol())
	.def_buffer([](shared_float_array &m) -> py::buffer_info{
	      return py::buffer_info(
	          const_cast<float*>(m.data()),                   /* Pointer to buffer */
	          sizeof(float),                                  /* Size of one scalar */
	          py::format_descriptor<float>::format(),         /* Python struct-style format descriptor */
	          m.dim(),                                        /* Number of dimensions */
	          get_dimensions(m),                              /* Buffer dimensions */
	          get_strides(m)                                  /* Strides (in bytes) for each index */
	            
	      );
	});
}


tf_activity_classifier_backend::tf_activity_classifier_backend(int batch_size, int num_features, 
	int num_classes, int predictions_in_chunk, const float_array_map& config, const float_array_map& weights) {

  	shared_float_array prediction_window = config.at("ac_pred_window");
  	const float * pred_window = prediction_window.data();
  	int pw = static_cast<int>(*pred_window);

    call_pybind_function([&]() {	
  	
	py::module tf_ac_backend = py::module::import("turicreate.toolkits.activity_classifier._tf_model_architecture");
	
	// Make an instance of python object 
	activity_classifier_ = tf_ac_backend.attr("ActivityTensorFlowModel")(weights, 
		batch_size, num_features, num_classes, pw, predictions_in_chunk);

    });

}


float_array_map tf_activity_classifier_backend::train(const float_array_map& inputs) {

	// Call train method on ActivityTensorflowModel
	float_array_map result;

	call_pybind_function([&]() {

		py::object output = activity_classifier_.attr("train")(inputs);

		
		std::map<std::string,py::buffer> buf_output = output.cast<std::map<std::string, py::buffer>>();

		
	  	for (auto& kv : buf_output){
	  		py::buffer_info buf = kv.second.request();
	  		turi::neural_net::shared_float_array value = turi::neural_net::shared_float_array::copy((float *)buf.ptr, std::vector<size_t>(buf.shape.begin(),buf.shape.end()));
	  		result[kv.first] = value;

	  	}
	
	});

	return result;

}

float_array_map tf_activity_classifier_backend::predict(const float_array_map& inputs) const {
	float_array_map result;

	// Call predict method on ActivityTensorFlowModel
	call_pybind_function([&]() {

		py::object output = activity_classifier_.attr("predict")(inputs);
		std::map<std::string,py::buffer> buf_output = output.cast<std::map<std::string, py::buffer>>();
		
	  	for (auto& kv : buf_output){
	  		py::buffer_info buf = kv.second.request();
	  		turi::neural_net::shared_float_array value = turi::neural_net::shared_float_array::copy((float *)buf.ptr, std::vector<size_t>(buf.shape.begin(),buf.shape.end()));
	  		result[kv.first] = value;

	  	}	
	
	});

	return result;
	
}

float_array_map tf_activity_classifier_backend::export_weights() const {
	float_array_map result;
	call_pybind_function([&]() {

		// Call export_weights method on ActivityTensorFLowModel
		py::object exported_weights = activity_classifier_.attr("export_weights")();
		std::map<std::string,py::buffer> buf_output = exported_weights.cast<std::map<std::string, py::buffer>>();
		
	  	for (auto& kv : buf_output){
	  		py::buffer_info buf = kv.second.request();
	  		turi::neural_net::shared_float_array value = turi::neural_net::shared_float_array::copy((float *)buf.ptr, std::vector<size_t>(buf.shape.begin(),buf.shape.end()));
	  		result[kv.first] = value;

	  	}
  	
	});

	return result;
	
}

void tf_activity_classifier_backend::set_learning_rate(float lr) {
	float_array_map result;

	// Call set_learning_rate method on ActivityTensorFLowModel
	call_pybind_function([&]() {

	activity_classifier_.attr("set_learning_rate")(lr);
	});
}	

tf_activity_classifier_backend::~tf_activity_classifier_backend() {
   call_pybind_function([&]() {
     activity_classifier_ = py::object();	
 });
}

} //neural_net
} // turi



