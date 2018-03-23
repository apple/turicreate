/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <stdio.h>
#include <png.h>
#include <string.h>
#include <logger/logger.hpp>

#ifndef png_infopp_NULL
#define png_infopp_NULL (png_infopp)NULL
#endif

#ifndef int_p_NULL
#define int_p_NULL (int*)NULL
#endif 

#include <boost/gil/extension/io/png_io.hpp>

namespace turi {

const size_t PNG_HEADER_SIZE = 4;
const size_t BIT_DEPTH = 8; 

/**
 * Simple struct for in-memory read_only source.
 */
struct png_memory_buffer {
  char* data; // the begin of the memory
  size_t length; // ths size of the data
  size_t offset; // the current read pointer
};

void png_memwrite_func(png_structp png_ptr, png_bytep data, png_size_t size)
{
  png_memory_buffer *png_buff = (png_memory_buffer*)png_get_io_ptr(png_ptr);
  size_t nsize = png_buff->length + size;

  if (png_buff->data){
    png_buff->data =(char*) realloc(png_buff->data, nsize);
  } else {
    png_buff->data =(char*) malloc(nsize);
  }

  if (!png_buff->data){
    png_error(png_ptr, "Write Error");
  }

  memcpy(png_buff->data + png_buff->length, data, size);
  png_buff->length += size;  

}

void png_mem_flush(png_structp png_ptr)
{
}

/**
 * Function for reading from png_memory_buffer
 */
void png_memread_func(png_structp png_ptr, png_bytep buf, png_size_t size){
  png_memory_buffer *png_buff = (png_memory_buffer*)png_get_io_ptr(png_ptr);
  if(png_buff == NULL)
    return;

  if (png_buff->offset + size <= png_buff->length) {
    memcpy(buf, png_buff->data + png_buff->offset, size);
    png_buff->offset += size;
  } else {
    return;
  }
}

/**
 * C++ error handler log and throwing "unexpected libpng error".
 */
static void png_error_handler(png_structp png_ptr,
                              png_const_charp msg) {
  logstream(LOG_ERROR) << "libpng error: " << msg << std::endl;
  png_destroy_read_struct(&png_ptr, NULL, NULL);
  log_and_throw(std::string("Unexpected libpng error"));
}

/**
 * Initialize png reader struct.
 */
void setup_png_reader(const char* data, size_t length, png_structp* outpng_ptr, png_infop* out_info_ptr) {
  if (length <= PNG_HEADER_SIZE || !png_check_sig((png_bytep)data, PNG_HEADER_SIZE)) {
    logstream(LOG_ERROR) << "Invalid PNG signature" << std::endl;
    throw(std::string("Invalid PNG file"));
  }

  // Begin setup
  // main png read struct
  png_structp png_ptr = NULL;
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_error_handler, NULL);
  if(png_ptr == NULL) {
    logstream(LOG_ERROR) << "Fail allocating PNG reader struct" << std::endl;
    throw(std::string("Unexpected libpng error"));
  }

  png_set_option(png_ptr, PNG_MAXIMUM_INFLATE_WINDOW,
                 PNG_OPTION_ON);

  // png info struct
  png_infop info_ptr = NULL;
  info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == NULL) {
    // libpng must free file info struct memory before we bail
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    logstream(LOG_ERROR) << "Fail allocating PNG info struct" << std::endl;
    throw(std::string("Unexpected libpng error"));
  }

  *outpng_ptr = png_ptr;
  *out_info_ptr = info_ptr;
}

/**
 * Convert number of channels to png_color_type
 */
int png_color_type(int num_channels){
  if (num_channels == 1){
    return PNG_COLOR_TYPE_GRAY;
  } else if (num_channels == 3) {
    return PNG_COLOR_TYPE_RGB;
  } else if (num_channels == 4) {
    return PNG_COLOR_TYPE_RGBA;
  } else {
    return -1;
  }
}

/*
 * Initialize png writer struct
 */
void setup_png_writer(png_structp* outpng_ptr, png_infop* out_info_ptr, size_t width, size_t height, size_t channels){

  //Begin setup
  // main png read struct
  png_structp png_ptr = NULL;
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, png_error_handler, NULL);
  if (png_ptr == NULL){
    logstream(LOG_ERROR) << "Fail allocating PNG writer struct" << std::endl;
    throw(std::string("Unexpected libpng error"));
  }

  //png info struct
  png_infop info_ptr = NULL;
  info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == NULL) {
    png_destroy_write_struct(&png_ptr,NULL);
    logstream(LOG_ERROR) << "Fail allocating PNG info struct" << std::endl;
    throw(std::string("Unexpected libpng error"));
  }

  int color_type = png_color_type(channels);

  png_set_IHDR(png_ptr, info_ptr, width, height, BIT_DEPTH, color_type, 
      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);


  *outpng_ptr = png_ptr;
  *out_info_ptr = info_ptr;

}

/**
 * Helper function: return number of channels from PNG color type.
 */
int png_num_channels(int color_type) {
  switch (color_type) {
  case PNG_COLOR_TYPE_GRAY:  // grayscale
    return 1;
  case PNG_COLOR_TYPE_RGB:  // rgb
  case PNG_COLOR_TYPE_PALETTE:  // indexed; will be translated to rgb
    return 3;
  case PNG_COLOR_TYPE_RGBA:  // rgb + alpha
    return 4;
  case PNG_COLOR_TYPE_GRAY_ALPHA:
    return 2;
  default:
    return -1;
  }
}

/**
 * Parse the png image info from in memory buffer.
 */
void parse_png(const char* data, size_t length,
               size_t& width, size_t& height,
               size_t& channels) {

  // Begin of setup
  png_structp png_ptr;
  png_infop info_ptr;
  setup_png_reader(data, length, &png_ptr, &info_ptr);

  // Construct the simple in memory buffer 
  png_memory_buffer source;
  source.data = (char*)data;
  source.length = length;
  source.offset = 0;

  // Set custom read function
  png_set_read_fn(png_ptr, (png_voidp)&source, (png_rw_ptr)png_memread_func);
  png_read_info(png_ptr, info_ptr);
  // End of setup

  png_uint_32 _width = 0;
  png_uint_32 _height = 0;
  int bit_depth = 0;
  int color_type = -1;
  png_uint_32 retval = png_get_IHDR(png_ptr, info_ptr,
      &_width,
      &_height,
      &bit_depth,
      &color_type,
      NULL, NULL, NULL);

  if(retval != 1) {
    logstream(LOG_ERROR) << "Fail parsing PNG header" << std::endl;
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    throw(std::string("Invalid PNG file"));
  }

  width = _width;
  height = _height;

  channels = png_num_channels(color_type);
  if (channels != 1 && channels != 3 && channels != 4) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    log_and_throw(std::string("Unsupported PNG color type: ") + std::to_string(color_type));
  }

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

void encode_png(const char* data ,size_t width, size_t height, size_t channels, char** out_data, size_t& out_length){
  //Beginning of setup 
  png_structp png_ptr;
  png_infop info_ptr;
  setup_png_writer(&png_ptr, &info_ptr, width, height, channels);

  //Reading the data in
  png_byte** row_pointers = (png_byte**) png_malloc(png_ptr, height * sizeof(png_byte *));
  for (size_t y = 0; y < height; ++y){
    char* row =(char*) png_malloc(png_ptr, sizeof(char) * channels * width);
    row_pointers[y] = (png_byte *) row; 
    for (size_t x = 0; x < width; ++x){
      for (size_t z = 0; z < channels; ++z){ 
        *row++ = *data++;
        }
      } 
    }

  // Construct in memory-buffer
  png_memory_buffer destination;
  destination.data = NULL;
  destination.length = 0;
  destination.offset = 0;

  // Writing decoded data into buffer 
  png_set_write_fn(png_ptr, &destination, png_memwrite_func, png_mem_flush);
  png_set_rows(png_ptr, info_ptr, row_pointers);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  //Copying buffer to output
  out_length = destination.length; 

  /**
   * destination.data is allocated using malloc realloc. This cannot
   * be used directly because `image_type`
   * requires C++ char[] for proper resource management.
   */
  // Make a copy into char[]
  *out_data = new char[out_length];
  memcpy(*out_data, destination.data, out_length);
  free(destination.data);

  //Cleanup
  for (size_t y = 0; y < height; y++){
    png_free (png_ptr, row_pointers[y]);
  }
  png_free(png_ptr, row_pointers);

  png_destroy_write_struct(&png_ptr, &info_ptr); 
}

void decode_png(const char* data, size_t length, char** out_data, size_t& out_length) {
  //Check for NULL data pointer
  if (data == NULL){
    log_and_throw("Trying to decode image with NULL data pointer");
  }
  
  // Begin of setup
  png_structp png_ptr;
  png_infop info_ptr;
  setup_png_reader(data, length, &png_ptr, &info_ptr);

  // Construct the simple in memory buffer 
  png_memory_buffer source;
  source.data = (char*)data;
  source.length = length;
  source.offset = 0;

  // Set custom read function
  png_set_read_fn(png_ptr, (png_voidp)&source, (png_rw_ptr)png_memread_func);
  png_read_info(png_ptr, info_ptr);
  // End of setup

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  png_get_IHDR(png_ptr, info_ptr, &width, &height,&bit_depth,&color_type,&interlace_type, NULL, NULL);

  // Handle 256-color indexed images (type P) by converting to RGB
  if (color_type == PNG_COLOR_TYPE_PALETTE) {
      png_set_palette_to_rgb(png_ptr);
  }

  // Handle bit depths smaller than 8
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
      png_set_expand_gray_1_2_4_to_8(png_ptr);
  }

  // Handle 16-bit depth by removing precision
  if (bit_depth == 16) {
#if PNG_LIBPNG_VER >= 10504
       png_set_scale_16(png_ptr);
#else
       png_set_strip_16(png_ptr);
#endif
  }

  int channels = png_num_channels(color_type);
  size_t row_stride = width * channels;
  out_length = width * height * channels;
  *out_data = new char[out_length];
  for (size_t i = 0; i < height; ++i) {
    png_read_row(png_ptr, (png_bytep)(*out_data + i * row_stride), NULL);
  }
  png_read_end(png_ptr,NULL);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

} // end of turicreate
