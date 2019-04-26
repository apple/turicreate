#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wrange-loop-analysis"

#ifdef check
#define __old_check check
#undef check
#endif

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#ifdef __old_check
#define check __old_check
#endif

#pragma clang diagnostic pop

#import <CoreML/CoreML.h>

#include "CoreMLPythonUtils.h"

#include <cstring>
#include <sstream>
#include <vector>

namespace py = pybind11;

@interface PybindCompatibleArray : MLMultiArray {
    // Holding reference to underlying memory
    py::array m_array;
}

- (PybindCompatibleArray *)initWithArray:(py::array)array;

@end
