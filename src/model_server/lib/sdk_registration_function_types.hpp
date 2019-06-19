/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SDK_REGISTRATION_FUNCTION_TYPES_HPP
#define TURI_UNITY_SDK_REGISTRATION_FUNCTION_TYPES_HPP
#include <vector>
#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/toolkit_class_specification.hpp>

namespace turi {

typedef std::vector<toolkit_function_specification> (*get_toolkit_function_registration_type)();
typedef std::vector<toolkit_class_specification> (*get_toolkit_class_registration_type)();

} // turicreate
#endif // TURI_UNITY_SDK_REGISTRATION_FUNCTION_TYPES_HPP
