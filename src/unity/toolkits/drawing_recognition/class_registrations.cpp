/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "class_registrations.hpp"
#include "data_preparation.hpp"

//#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>

namespace turi {
namespace sdk_model {
namespace drawing_recognition {

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_drawing_recognition_prepare_data , "data" , "feature" , "target")
END_FUNCTION_REGISTRATION

}// drawing_recognition
}// sdk_model
}// turicreate
