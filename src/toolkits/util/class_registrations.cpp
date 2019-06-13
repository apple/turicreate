/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_macros.hpp>
#include <toolkits/util/class_registrations.hpp>
#include <toolkits/util/random_sframe_generation.hpp>

namespace turi {
namespace util {

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_generate_random_sframe,
                  "num_rows", "column_codes", "random_seed", "generate_target", "noise_level");

REGISTER_FUNCTION(_generate_random_classification_sframe,
                  "num_rows", "column_codes", "random_seed", "num_classes",
                  "num_extra_class_bins", "misclassification_spread");

END_FUNCTION_REGISTRATION
}  // namespace util
}  // namespace turi
