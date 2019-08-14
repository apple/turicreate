/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_PYTHON_MODEL_ADDONE
#define TURI_PYTHON_MODEL_ADDONE
#include <vector>
#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/toolkit_class_specification.hpp>

namespace turi {
namespace python_model {

/**
 * Obtains the registration for the PythonModel.
 */
std::vector<toolkit_class_specification> get_toolkit_class_registration();

} // namespace python_model
} // namespace turi
#endif
