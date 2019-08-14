/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/unity_global_singleton.hpp>
#include <model_server/lib/unity_global.hpp>

namespace turi {

std::shared_ptr<unity_global> unity_global_ptr;

void create_unity_global_singleton(toolkit_function_registry* _toolkit_functions,
                                   toolkit_class_registry* _classes) {

  unity_global_ptr = std::make_shared<unity_global>(_toolkit_functions, _classes);
}

std::shared_ptr<unity_global> get_unity_global_singleton() {
  if (unity_global_ptr == nullptr) {
    ASSERT_MSG(false, "Unity Global has not been created");
  }
  return unity_global_ptr;
}

} // namespace turi
