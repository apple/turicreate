/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_REGSTRATION_HPP_
#define TURI_UNITY_REGSTRATION_HPP_

#include <model_server/lib/toolkit_class_registry.hpp>
#include <model_server/lib/toolkit_function_registry.hpp>

namespace turi {

  void register_functions(toolkit_function_registry& registry);
  void register_models(toolkit_class_registry& registry);

}

#endif
