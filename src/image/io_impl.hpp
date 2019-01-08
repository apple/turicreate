/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_IMAGE_IMAGE_IO_IMPL_HPP
#define TURI_IMAGE_IMAGE_IO_IMPL_HPP

#ifndef png_infopp_NULL
#define png_infopp_NULL (png_infopp)NULL
#endif

#ifndef int_p_NULL
#define int_p_NULL (int*)NULL
#endif 

#include <image/image_type.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/jpeg/old.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/io/png/old.hpp>
#include <logger/logger.hpp>

using namespace boost::gil;

namespace turi {

template<typename pixel_type>
void write_image_impl(std::string filename, char* data, size_t& width, size_t& height, size_t& channels, Format format ) {
  auto view = interleaved_view(width, height, (pixel_type*)data, width * channels * sizeof(char));
  if (format == Format::JPG) {
    jpeg_write_view(filename, view);
  } else  if (format == Format::PNG){
    png_write_view(filename, view);
  }
} 

// Template specialization: JPEG does not support RGBA
template<>
void write_image_impl<rgba8_pixel_t>(std::string filename, char* data, size_t& width, size_t& height, size_t& channels, Format format ) {
  auto view = interleaved_view(width, height, (rgba8_pixel_t*)data, width * channels * sizeof(char));
  if (format == Format::JPG) {
    throw ("JPEG does not support RGBA color type");
  } else  if (format == Format::PNG){
    png_write_view(filename, view);
  }
}

/**************************************************************************/
/*                                                                        */
/*             Boost Prototype Code, Not used in actual code              */
/*                                                                        */
/**************************************************************************/
template<typename pixel_type>
void boost_read_image_impl(std::string filename, char** data, size_t& width, size_t& height, size_t& channels, Format format ) {
  char* buf = new char[width * height * channels];
  auto view = interleaved_view(width, height, (pixel_type*)buf, width * channels * sizeof(char));
  if (format == Format::JPG) {
    jpeg_read_view(filename, view);
  } else  if (format == Format::PNG){
    png_read_view(filename, view);
  }
  *data = buf;
}

// Template specialization: JPEG does not support RGBA
template<>
void boost_read_image_impl<rgba8_pixel_t>(std::string filename, char** data, size_t& width, size_t& height, size_t& channels, Format format ) {
  char* buf = new char[width * height * channels];
  auto view = interleaved_view(width, height, (rgba8_pixel_t*)buf, width * channels * sizeof(char));
  if (format == Format::JPG) {
    throw ("JPEG does not support RGBA color type");
  } else  if (format == Format::PNG){
    png_read_view(filename, view);
  }
  *data = buf;
}


}
#endif
