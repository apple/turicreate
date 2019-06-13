/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FEATURE_ENGINEERING_REGISTRATIONS_H_
#define FEATURE_ENGINEERING_REGISTRATIONS_H_


#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_class_specification.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();

}// feature_engineering
}// sdk_model
}// turicreate
#endif
