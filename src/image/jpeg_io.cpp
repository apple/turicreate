/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// logger must be included before jpeglib
// this is due to an annoying reason
//
// On windows:
// logger include pthread which includes windows.h which typedefs INT32
// 
// jpeglib.h also typedefs INT32
//
// jpeglib.h however puts some guard protection to avoid redefining INT32
// but of course that only works if I include windows BEFORE jpeglib.h
//
#include <logger/logger.hpp>
#include <stdio.h>
#include <jpeglib.h>

#include <string.h>

namespace turi {

/*
 * Here's the routine that will replace the standard error_exit method:
 */
METHODDEF(void)
jpeg_error_exit (j_common_ptr cinfo)
{
  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);
  log_and_throw(std::string("Unexpected JPEG decode failure"));
}

void parse_jpeg(const char* data, size_t length,
                size_t& width, size_t& height,
                size_t& channels) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  memset(&cinfo, 0, sizeof(cinfo));
  memset(&jerr, 0, sizeof(jerr));
  // Initialize the JPEG decompression object with default error handling
  cinfo.err = jpeg_std_error(&jerr);
  jerr.error_exit = jpeg_error_exit;
  try {
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, (unsigned char*)data, length); // Specify data source for decompression
    jpeg_read_header(&cinfo, TRUE); // Read file header, set default decompression parameters

    if (cinfo.out_color_space != JCS_GRAYSCALE && cinfo.out_color_space != JCS_RGB) {
      log_and_throw(std::string("Unsupported colorspace format. Currently, only RGB and Grayscale are supported."));
    }

    width = cinfo.image_width;
    height = cinfo.image_height;
    channels = cinfo.num_components;
  } catch (...) {
    jpeg_destroy_decompress(&cinfo);
    throw;
  }
  jpeg_destroy_decompress(&cinfo);
}

void decode_jpeg(const char* data, size_t length, char** out_data, size_t& out_length) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  memset(&cinfo, 0, sizeof(cinfo));
  memset(&jerr, 0, sizeof(jerr));
  // Initialize the JPEG decompression object with default error handling
  cinfo.err = jpeg_std_error(&jerr);
  jerr.error_exit = jpeg_error_exit;
  *out_data = NULL;
  out_length = 0;

  if (data == NULL) {
    log_and_throw("Trying to decode image with NULL data pointer.");
  }

  try {
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, (unsigned char*)data, length); // Specify data source for decompression
    jpeg_read_header(&cinfo, TRUE); // Read file header, set default decompression parameters
    jpeg_start_decompress(&cinfo); // Start decompressor

    size_t width = cinfo.image_width;
    size_t height = cinfo.image_height;
    size_t channels = cinfo.num_components;
    out_length = width * height * channels;
    *out_data = new char[out_length];
    size_t row_stride = width * channels;

    size_t row_offset = 0;
    JSAMPROW rowptr[1];
    while (cinfo.output_scanline < cinfo.output_height) {
      rowptr[0] = (unsigned char*)(*out_data + row_offset * row_stride);
      jpeg_read_scanlines(&cinfo, rowptr, 1);
      ++row_offset;
    }

    jpeg_finish_decompress(&cinfo);
  } catch (...) {
    if (*out_data != NULL) {
      delete[] *out_data;
      out_length = 0;
    }
    jpeg_destroy_decompress(&cinfo);
    throw;
  }
  jpeg_destroy_decompress(&cinfo);
}

}
