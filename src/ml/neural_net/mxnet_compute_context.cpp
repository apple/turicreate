/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
*/

#include <iostream>
#include <vector>

#include <core/export.hpp>
#include <external/pybind11/pybind11.h>
#include <external/pybind11/stl.h>
#include <external/pybind11/numpy.h>
#include <ml/neural_net/mxnet_compute_context.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>


namespace py = pybind11;