/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "class_registrations.hpp"
#include "sequence_iterator.hpp"
#include "activity_classifier.hpp"

//#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>

namespace turi {
namespace sdk_model {
namespace activity_classification {

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_activity_classifier_prepare_data , "data" , "features" , "session_id" , "prediction_window" , "predictions_in_chunk" , "target")
REGISTER_FUNCTION(_activity_classifier_prepare_data_verbose , "data" , "features" , "session_id" , "prediction_window" , "predictions_in_chunk" , "target")
END_FUNCTION_REGISTRATION

}// activity_classification
}// sdk_model
}// turicreate
