/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <model_server/lib/toolkit_class_macros.hpp>
#include <toolkits/object_detection/one_shot_object_detection/class_registrations.hpp>
#include <toolkits/object_detection/one_shot_object_detection/one_shot_object_detector.hpp>

namespace turi {
namespace one_shot_object_detection {

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(one_shot_object_detector)
END_CLASS_REGISTRATION

}  // namespace one_shot_object_detection
}  // namespace turi
