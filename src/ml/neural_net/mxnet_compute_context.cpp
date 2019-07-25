/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
*/


#include <include/pybind11/pybind11.h>
#include <include/pybind11/embed.h>
#include <iostream>
#include "mxnet_compute_context.hpp"
#include <core/export.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <vector>
#include <include/pybind11/stl.h>
#include <include/pybind11/numpy.h>


namespace py = pybind11;

py::array_t<double> add_arrays(py::array_t<double> input1, py::array_t<double> input2) {
    py::buffer_info buf1 = input1.request(), buf2 = input2.request();

    if (buf1.ndim != 1 || buf2.ndim != 1)
        throw std::runtime_error("Number of dimensions must be one");

    if (buf1.size != buf2.size)
        throw std::runtime_error("Input shapes must match");

    /* No pointer is passed, so NumPy will allocate the buffer */
    auto result = py::array_t<double>(buf1.size);

    py::buffer_info buf3 = result.request();

    double *ptr1 = (double *) buf1.ptr,
           *ptr2 = (double *) buf2.ptr,
           *ptr3 = (double *) buf3.ptr;

    for (size_t idx = 0; idx < 3; idx++)
        ptr3[idx] = ptr1[idx] + ptr2[idx];

    return result;
}

// int import_modules() {
	
// 	py::module calc = py::module::import("py_mod.calc");
// 	py::object result = calc.attr("add")(1, 2);
// 	int n = result.cast<int>();
	
// 	return n;
// }

py::array_t<double> import_obj(){
	std::cout << "hey";
	py::module calc = py::module::import("py_mod.calc");
	py::object res = calc.attr("tran")();
	py::array_t<double> n = res.cast<py::array_t<double>>();
	return n;

}

PYBIND11_MODULE(libtcmxnet, m) {
	
    m.doc() = "pybind11 example module";
    m.def("add_arrays", &add_arrays, "Add two NumPy arrays");
    // m.def("import_modules", &import_modules, "Calc");
    m.def("import_obj", &import_obj, "ghjjk");
}





