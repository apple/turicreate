/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/image_util.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>

namespace turi{

namespace image_util{


BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(load_image, "url", "format")
REGISTER_FUNCTION(load_images, "url", "format", "with_path", "recursive", "ignore_failure", "random_order")
REGISTER_FUNCTION(decode_image, "image")
REGISTER_FUNCTION(decode_image_sarray, "image_sarray")
REGISTER_FUNCTION(resize_image, "image",  "resized_width", "resized_height", "resized_channels", "decode", "resample_method")
REGISTER_FUNCTION(resize_image_sarray, "image_sarray",  "resized_width", "resized_height", "resized_channels", "decode", "resample_method")
REGISTER_FUNCTION(vector_sarray_to_image_sarray, "sarray",  "width", "height", "channels", "undefined_on_failure")
REGISTER_FUNCTION(generate_mean, "unity_data")
END_FUNCTION_REGISTRATION



} //namespace image_util
} //namespace turi
