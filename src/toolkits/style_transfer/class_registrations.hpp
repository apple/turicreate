/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_STYLE_TRANSFER_REGISTRATION_H_
#define TURI_STYLE_TRANSFER_REGISTRATION_H_

#include <model_server/lib/toolkit_function_macros.hpp>

namespace turi {
namespace style_transfer {

std::vector<toolkit_class_specification> get_toolkit_class_registration();

}  // namespace style_transfer
}  // namespace turi

#endif