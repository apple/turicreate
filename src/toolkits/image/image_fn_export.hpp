/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef IMAGE_FN_EXPORT_HPP
#define IMAGE_FN_EXPORT_HPP

#include <model_server/lib/toolkit_function_specification.hpp>

namespace turi{

namespace image_util{

std::vector<toolkit_function_specification>
  get_toolkit_function_registration();


} // end of image_util
} // end of turi

#endif //  IMAGE_FN_EXPORT_HPP
