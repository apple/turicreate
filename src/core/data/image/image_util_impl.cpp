/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/data/image/io.hpp>
#ifndef png_infopp_NULL
#define png_infopp_NULL (png_infopp)NULL
#endif

#ifndef int_p_NULL
#define int_p_NULL (int*)NULL
#endif

#include <boost/gil/gil_all.hpp>
#include <core/data/image/numeric_extension/sampler.hpp>
#include <core/data/image/numeric_extension/resample.hpp>

namespace turi {


namespace image_util_detail {

using namespace boost::gil;

template<typename current_pixel_type, typename new_pixel_type>
void resize_image_detail(const char* data, size_t width, size_t height, size_t channels, size_t resized_width, size_t resized_height, size_t resized_channels, char** resized_data, int resample_method){
  if (data == NULL){
    log_and_throw("Trying to resize image with NULL data pointer");
  }
  size_t len = resized_height * resized_width * resized_channels;
  char* buf = new char[len];
  // Fast path when the sizes are equal.
  if ((width == resized_width) && (height == resized_height) && (channels == resized_channels)) {
    memcpy(buf, data, len);
  } else {
    auto view = interleaved_view(width, height, (current_pixel_type*)data, width * channels * sizeof(char));
    auto resized_view = interleaved_view(resized_width, resized_height, (new_pixel_type*)buf,
                                         resized_width * resized_channels * sizeof(char));
    if (resample_method == 0) {
      resize_view(color_converted_view<new_pixel_type>(view), (resized_view), nearest_neighbor_sampler());
    } else if (resample_method == 1) {
      resize_view(color_converted_view<new_pixel_type>(view), (resized_view), bilinear_sampler());
    } else {
      log_and_throw("Unknown resampling method");
    }
  }
  *resized_data = buf;
}


/**
 * Resize the image, and set resized_data to resized image data.
 */
void resize_image_impl(const char* data, size_t width, size_t height, size_t channels, size_t resized_width, size_t resized_height, size_t resized_channels, char** resized_data, int resample_method) {
  // This code should be simplified
  if (channels == 1) {
    if (resized_channels == 1){
      resize_image_detail<gray8_pixel_t, gray8_pixel_t>(data, width, height, channels,
                                                      resized_width, resized_height,
                                                      resized_channels, resized_data,
                                                      resample_method);
    } else if (resized_channels == 3){
      resize_image_detail<gray8_pixel_t, rgb8_pixel_t>(data, width, height, channels,
                                                     resized_width, resized_height,
                                                     resized_channels, resized_data,
                                                     resample_method);
    } else if (resized_channels == 4){
      resize_image_detail<gray8_pixel_t, rgba8_pixel_t>(data, width, height, channels,
                                                      resized_width, resized_height,
                                                      resized_channels, resized_data,
                                                      resample_method);
    } else {
      log_and_throw (std::string("Unsupported channel size ") + std::to_string(channels));
    }
  } else if (channels == 3) {
    if (resized_channels == 1){
      resize_image_detail<rgb8_pixel_t, gray8_pixel_t>(data, width, height, channels,
                                                      resized_width, resized_height,
                                                      resized_channels, resized_data,
                                                      resample_method);
    } else if (resized_channels == 3){
      resize_image_detail<rgb8_pixel_t, rgb8_pixel_t>(data, width, height, channels,
                                                     resized_width, resized_height,
                                                     resized_channels, resized_data,
                                                     resample_method);
    } else if (resized_channels == 4){
      resize_image_detail<rgb8_pixel_t, rgba8_pixel_t>(data, width, height, channels,
                                                      resized_width, resized_height,
                                                      resized_channels, resized_data,
                                                      resample_method);
    } else {
      log_and_throw (std::string("Unsupported channel size ") + std::to_string(channels));
    }
  } else if (channels == 4) {
    if (resized_channels == 1){
      resize_image_detail<rgba8_pixel_t, gray8_pixel_t>(data, width, height, channels,
                                                      resized_width, resized_height,
                                                      resized_channels, resized_data,
                                                      resample_method);
    } else if (resized_channels == 3){
      resize_image_detail<rgba8_pixel_t, rgb8_pixel_t>(data, width, height, channels,
                                                     resized_width, resized_height,
                                                     resized_channels, resized_data,
                                                     resample_method);
    } else if (resized_channels == 4){
      resize_image_detail<rgba8_pixel_t, rgba8_pixel_t>(data, width, height, channels,
                                                      resized_width, resized_height,
                                                      resized_channels, resized_data,
                                                      resample_method);
    } else {
      log_and_throw (std::string("Unsupported channel size ") + std::to_string(channels));
    }
  }
}

void decode_image_impl(image_type& image) {
  if (image.m_format == Format::RAW_ARRAY) {
    return;
  }

  char* buf = NULL;
  size_t length = 0;

  if (image.m_format == Format::JPG) {
    decode_jpeg((const char*)image.get_image_data(), image.m_image_data_size,
                &buf, length);
  } else if (image.m_format == Format::PNG) {
    decode_png((const char*)image.get_image_data(), image.m_image_data_size,
                &buf, length);
  } else {
    log_and_throw(std::string("Cannot decode image. Unknown format."));
  };
  image.m_image_data.reset(buf);
  image.m_image_data_size = length;
  image.m_format = Format::RAW_ARRAY;
}

void encode_image_impl(image_type& image) {
  if (image.m_format != Format::RAW_ARRAY){
    return;
  }

  char* buf = NULL;
  size_t length = 0;

  encode_png((const char*)image.get_image_data(), image.m_width, image.m_height, image.m_channels, &buf, length);
  image.m_image_data.reset(buf);
  image.m_image_data_size = length;
  image.m_format = Format::PNG;
}

} // end of image_util_detail


void decode_image_inplace(image_type& image) {
  image_util_detail::decode_image_impl(image);
}

void encode_image_inplace(image_type& image) {
  image_util_detail::encode_image_impl(image);
}


} // end of turicreate
