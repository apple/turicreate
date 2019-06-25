/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_GLOBAL_SINGLETON_HPP
#define TURI_UNITY_GLOBAL_SINGLETON_HPP

#include <memory>
#include <model_server/lib/variant.hpp>

namespace turi {
class toolkit_function_registry;
class toolkit_class_registry;
class unity_global;


/**
 * Creates the unity_global singleton, passing in the arguments into the
 * unity_global constructor
 */
void create_unity_global_singleton(toolkit_function_registry* _toolkit_functions,
                                   toolkit_class_registry* _classes);

/**
 * Gets a pointer to the unity global singleton
 */
std::shared_ptr<unity_global> get_unity_global_singleton();


}
#endif // TURI_UNITY_GLOBAL_SINGLETON_HPP
