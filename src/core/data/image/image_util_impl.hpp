/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef IMAGE_UTIL_IMPL_HPP
#define IMAGE_UTIL_IMPL_HPP

#include <core/data/image/io.hpp>
namespace turi {


namespace image_util_detail {

void resize_image_impl(const char* data, size_t width, size_t height,
                       size_t channels, size_t resized_width, size_t resized_height,
                       size_t resized_channels, char** resized_data, int resample_method);

void decode_image_impl(image_type& image);

void encode_image_impl(image_type& image);

} // end of image_util_detail


/**
 * Makes an image raw.
 */
void decode_image_inplace(image_type& image);


/**
 * Makes an image png if raw.
 */
void encode_image_inplace(image_type& image);

} // end of turicreate
#endif
