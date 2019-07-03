/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/activity_classification/class_registrations.hpp>

#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <toolkits/activity_classification/ac_data_iterator.hpp>
#include <toolkits/activity_classification/activity_classifier.hpp>

namespace turi {
namespace activity_classification {

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_activity_classifier_prepare_data , "data" , "features" , "session_id" , "prediction_window" , "predictions_in_chunk" , "target")
REGISTER_FUNCTION(_activity_classifier_prepare_data_verbose , "data" , "features" , "session_id" , "prediction_window" , "predictions_in_chunk" , "target")
REGISTER_NAMED_FUNCTION("_activity_classifier_random_split_by_session", activity_classifier::random_split_by_session, "data","session_id", "fraction", "seed");
REGISTER_DOCSTRING(
    activity_classifier::random_split_by_session,
    "----------\n"
    "data : SFrame\n"
    "    Dataset of new observations. Must include columns with the same\n"
    "    names as the features used for model training.\n"
    "session_id : string\n"
    "    Name of the column that contains a unique ID for each session.\n"
    "fraction : float, optional\n"
    "   The dataset is randomly split into two datasets where one contains\n"
    "   data for a fraction of the sessions while the second contains the\n"
    "   rest of the sessions. The value can vary between 0 to 1.\n"
    "seed : int\n"
    "   Seed value is used as a base to generate a random number. If you "
    "provide\n"
    "   same seed value before generating random data it will produce the "
    "same data.\n");
END_FUNCTION_REGISTRATION

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(activity_classifier)
END_CLASS_REGISTRATION

}  // activity_classification
}  // turi
