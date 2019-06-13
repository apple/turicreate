/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/variant.hpp>
#include <model_server/lib/unity_global_singleton.hpp>
#include <model_server/lib/unity_global.hpp>

namespace turi {
namespace variant_converter_impl {
std::function<variant_type(const std::vector<variant_type>&)>
    get_toolkit_function_from_closure(const function_closure_info& closure) {
  auto native_execute_function = get_unity_global_singleton()
      ->get_toolkit_function_registry()
      ->get_native_function(closure);

  return native_execute_function;
}
} // variant_converter_impl
} // nturicreate
