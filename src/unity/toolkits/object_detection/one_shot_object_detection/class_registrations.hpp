/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_ONE_SHOT_OBJECT_DETECTION_CLASS_REGISTRATIONS
#define TURI_ONE_SHOT_OBJECT_DETECTION_CLASS_REGISTRATIONS

#include <vector>

#include <unity/lib/toolkit_class_specification.hpp>

namespace turi {
namespace one_shot_object_detection {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();

}// one_shot_object_detection
}// turicreate

#endif // TURI_ONE_SHOT_OBJECT_DETECTION_CLASS_REGISTRATIONS
