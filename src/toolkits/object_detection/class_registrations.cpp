/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/object_detection/class_registrations.hpp>

#include <model_server/lib/toolkit_class_macros.hpp>
#include <toolkits/object_detection/object_detector.hpp>

namespace turi {
namespace object_detection {

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(object_detector)
END_CLASS_REGISTRATION

}  // namespace object_detection
}  // namespace turi
