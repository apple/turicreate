/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_OBJECT_DETECTION_CLASS_REGISTRATIONS
#define TURI_OBJECT_DETECTION_CLASS_REGISTRATIONS

#include <vector>

#include <model_server/lib/toolkit_class_specification.hpp>

namespace turi {
namespace object_detection {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();

}  // object_detection
}  // turi

#endif  // TURI_OBJECT_DETECTION_CLASS_REGISTRATIONS
