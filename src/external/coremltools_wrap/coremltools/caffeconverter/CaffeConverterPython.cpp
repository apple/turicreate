#include "CaffeConverterLib.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wrange-loop-analysis"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"

namespace py = pybind11;

PYBIND11_PLUGIN(libcaffeconverter) {
    py::module m("libcaffeconverter", "C++ Caffe converter implementation");
    m.def("_convert_to_file", &convertCaffe, "Convert a Caffe model to mlmodel format.");
    return m.ptr();
}

#pragma clang diagnostic pop
