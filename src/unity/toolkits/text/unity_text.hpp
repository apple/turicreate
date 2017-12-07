/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TEXT_H
#define TURI_UNITY_TEXT_H

#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/unity_base_types.hpp>

namespace turi {
namespace text {

/**
 *
 */
toolkit_function_response_type init(toolkit_function_invocation& invoke);

/**
 */
toolkit_function_response_type train(toolkit_function_invocation& invoke);

/**
 */
toolkit_function_response_type get_topic(toolkit_function_invocation& invoke);

/**
 *
 */
toolkit_function_response_type predict(toolkit_function_invocation& invoke);

/**
 */
toolkit_function_response_type summary(toolkit_function_invocation& invoke);

/**
 */
toolkit_function_response_type get_training_stats(toolkit_function_invocation& invoke);


std::vector<toolkit_function_specification> get_toolkit_function_registration();

}
}

#endif /* TURI_UNITY_TEXT_H */
