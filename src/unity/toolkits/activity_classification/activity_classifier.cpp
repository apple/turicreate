/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/activity_classification/activity_classifier.hpp>

namespace turi {
namespace activity_classification {

void activity_classifier::train(gl_sframe data, std::string target_column_name,
                                std::string session_id_column_name,
                                variant_type validation_data,
                                std::map<std::string, flexible_type> opts) {
}

}  // namespace activity_classification
}  // namespace turi
