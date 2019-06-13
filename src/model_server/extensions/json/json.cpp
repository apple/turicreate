/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "decoder.hpp"
#include "encoder.hpp"

#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_class_macros.hpp>

#include <iostream>

using namespace turi;

static flexible_type _test_flexible_type(const flexible_type& value) {
  std::cout << "flexible_type type is ";
  std::cout << flex_type_enum_to_name(value.get_type()) << std::endl;
  std::cout << "flexible_type value is ";
  std::cout << value;
  return value;
}

BEGIN_FUNCTION_REGISTRATION;
REGISTER_NAMED_FUNCTION("json.to_serializable", JSON::to_serializable, "input");
REGISTER_NAMED_FUNCTION("json.from_serializable", JSON::from_serializable, "data", "schema");
REGISTER_NAMED_FUNCTION("json._test_flexible_type", _test_flexible_type, "input");
END_FUNCTION_REGISTRATION;
