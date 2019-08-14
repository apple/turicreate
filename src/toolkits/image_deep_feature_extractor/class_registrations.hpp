/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_IMAGE_DEEP_FEATURE_EXTRACTOR_REGISTRATIONS
#define TURI_IMAGE_DEEP_FEATURE_EXTRACTOR_REGISTRATIONS

#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_class_specification.hpp>

namespace turi {
namespace image_deep_feature_extractor {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();

}  // image_deep_feature_extractor
}  // turi

#endif
