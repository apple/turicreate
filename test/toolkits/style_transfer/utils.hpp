/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef __TC_TEST_TOOLKITS_STYLE_TRANSFER_UTILS
#define __TC_TEST_TOOLKITS_STYLE_TRANSFER_UTILS

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/image/image_type.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>

#include <string>


namespace turi {
namespace style_transfer {

std::string generate_data(size_t data_size);
turi::flex_image random_image();
turi::gl_sarray random_image_sarray(size_t length);
turi::gl_sframe random_sframe(size_t length, std::string image_column_name);

} // namespace style_transfer
} // namespace turi

#endif // __TC_TEST_TOOLKITS_STYLE_TRANSFER_UTILS