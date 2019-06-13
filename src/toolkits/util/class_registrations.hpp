/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TOOLKIT_UTIL_REGISTRATIONS_H_
#define TURI_TOOLKIT_UTIL_REGISTRATIONS_H_


#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_function_specification.hpp>

namespace turi {
namespace util {

std::vector<turi::toolkit_function_specification> get_toolkit_function_registration();

}  // namespace util
}  // namespace turi
#endif
