/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <model_server/lib/image_util.hpp>
#include <core/export.hpp>

extern "C" {

/******************************************************************************/
/*                                                                            */
/*    flex_image                                                              */
/*                                                                            */
/******************************************************************************/

struct tc_flex_image_struct;
typedef struct tc_flex_image_struct tc_flex_image;

// Load an image into a flexible type from a path
EXPORT tc_flex_image* tc_flex_image_create_from_path(const char* path,
                                                     const char* format,
                                                     tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  turi::flexible_type image_type = turi::image_util::load_image(path, format);
  return new_tc_flex_image(image_type.get<turi::flex_image>());

  ERROR_HANDLE_END(error, NULL);
}

// Load an image into a flexible type from raw data
EXPORT tc_flex_image* tc_flex_image_create_from_data(
    const char* data, uint64_t height, uint64_t width, uint64_t channels,
    uint64_t total_data_size, const char* format, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  turi::Format format_enum; 

  std::string fmt_s(format);
  if(fmt_s == "jpg" || fmt_s == "JPG") { 
    format_enum = turi::Format::JPG;
  } else if(fmt_s == "png" || fmt_s == "PNG") { 
    format_enum = turi::Format::PNG;  
  } else if (fmt_s == "RAW" || fmt_s == "raw") { 
    format_enum = turi::Format::RAW_ARRAY;  
  } else { 
    throw std::invalid_argument(
        "format string must be one of \"jpg\", \"png\", or \"raw\"."); 
  }

  return new_tc_flex_image(data, height, width, channels, total_data_size,
                           IMAGE_TYPE_CURRENT_VERSION, static_cast<int>(format_enum));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_flex_image_width(const tc_flex_image* image, tc_error** error) { 
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, image, "image", 0); 

  return image->value.m_width;

  ERROR_HANDLE_END(error, 0);
}


EXPORT uint64_t tc_flex_image_height(const tc_flex_image* image, tc_error** error) { 
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, image, "image", 0); 

  return image->value.m_height;

  ERROR_HANDLE_END(error, 0);
}
 
EXPORT uint64_t tc_flex_image_num_channels(const tc_flex_image* image, tc_error** error) { 
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, image, "image", 0); 

  return image->value.m_channels;

  ERROR_HANDLE_END(error, 0);
}
 
EXPORT uint64_t tc_flex_image_data_size(const tc_flex_image* image, tc_error** error) { 
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, image, "image", 0); 

  return image->value.m_image_data_size;

  ERROR_HANDLE_END(error, 0);
}
 
EXPORT const char* tc_flex_image_data(const tc_flex_image* image, tc_error** error) { 
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, image, "image", NULL); 

  return image->value.m_image_data.get();

  ERROR_HANDLE_END(error, NULL);
}
 
EXPORT const char* tc_flex_image_format(const tc_flex_image* image, tc_error** error) { 
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, image, "image", NULL); 

  switch(image->value.m_format) { 
    case turi::Format::JPG: return "jpg";
    case turi::Format::PNG: return "png";
    case turi::Format::RAW_ARRAY: return "raw";
    case turi::Format::UNDEFINED: return "undefined";
    default:
      log_and_throw(std::string("Unrecognized image format"));
  }

  ERROR_HANDLE_END(error, NULL);
}

}
