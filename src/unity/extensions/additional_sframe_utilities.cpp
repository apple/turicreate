/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <vector>
#include <parallel/pthread_tools.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <sframe/sframe_config.hpp>
#include <image/image_type.hpp>
#include <image/io.hpp>
#include "additional_sframe_utilities.hpp"
using namespace turi;


template <typename T>
void copy_image_to_memory(const image_type& img, T *outptr,
                          const std::vector<size_t>& outstrides,
                          bool channel_last) {
  ASSERT_EQ(outstrides.size(), 3);
  size_t index_h, index_w, index_c;
  if (channel_last) {
    // Format: HWC
    index_h = 0;
    index_w = 1;
    index_c = 2;
  } else {
    // Format: CHW
    index_c = 0;
    index_h = 1;
    index_w = 2;
  }

  // Decode if needed
  if (!img.is_decoded()) {
    char* buf = NULL;
    size_t length = 0;
    if (img.m_format == Format::JPG) {
      decode_jpeg((const char*)img.get_image_data(), img.m_image_data_size, &buf, length);
    } else if (img.m_format == Format::PNG) {
      decode_png((const char*)img.get_image_data(), img.m_image_data_size, &buf, length);
    } else {
      ASSERT_MSG(false, "Unsupported image format");
    }
    size_t cnt = 0;
    for (size_t i = 0; i < img.m_height; ++i) {
      for (size_t j = 0; j < img.m_width; ++j) {
        for (size_t k = 0; k < img.m_channels; ++k) {
          outptr[i * outstrides[index_h] + j * outstrides[index_w] + k * outstrides[index_c]] = static_cast<unsigned char>(buf[cnt++]);
        }
      }
    }
    delete[] buf;
  } else {
    size_t cnt = 0;
    const unsigned char* raw_data = img.get_image_data();
    for (size_t i = 0; i < img.m_height; ++i) {
      for (size_t j = 0; j < img.m_width; ++j) {
        for (size_t k = 0; k < img.m_channels; ++k) {
          outptr[i * outstrides[index_h] + j * outstrides[index_w] + k * outstrides[index_c]] = static_cast<unsigned char>(raw_data[cnt++]);
        }
      }
    }
  }
}

void copy_to_memory(const sframe_rows::row& data,
                    float* outptr, 
                    const std::vector<size_t>& outstrides,
                    const std::vector<size_t>& field_length_p) {
  ASSERT_GE(data.size(), 1);

  for (size_t i = 0; i < data.size(); ++i) {
    ASSERT_NE((int)data[i].get_type(), (int)flex_type_enum::UNDEFINED);
  }

  // Case 1: Image type
  auto type = data[0].get_type();
  if (type == flex_type_enum::IMAGE) {
    ASSERT_MSG(data.size() == 1, "Image data only support one input field");
    const image_type& img = data[0].get<flex_image>();
    copy_image_to_memory<float>(img, outptr, outstrides, false);
    return;
  } else if (data.size() == 1 && (type == flex_type_enum::FLOAT || type == flex_type_enum::INTEGER)) {
    // Case 2: Single value type (should really get rid of this special case) 
    ASSERT_EQ(outstrides.size(), 0);
    (*outptr) = (float)(data[0]);
    return;
  } else if (data.size() == 1 && type == flex_type_enum::LIST) {
    // Case 3: 2D arrays: list of vectors or list of lists of values
    // field_length defines shape of the 2d array
    ASSERT_EQ(field_length_p.size(), 2);
    const flex_list& dim0_lst = data[0].to<flex_list>();
    ASSERT_EQ(dim0_lst.size(), field_length_p[0]);
    for (size_t i = 0; i < dim0_lst.size(); ++i) {
      auto dim1_type = dim0_lst[i].get_type();
      if (dim1_type == flex_type_enum::VECTOR) {
        const flex_vec& dim1_vec = dim0_lst[i].to<flex_vec>();
        ASSERT_EQ(dim1_vec.size(), field_length_p[1]);
        for (size_t j = 0; j < dim1_vec.size(); ++j) {
          outptr[outstrides[0] * i + outstrides[1] * j] = (float)(dim1_vec[j]);
        }
      } else if (dim1_type == flex_type_enum::LIST) {
        const flex_list& dim1_lst = dim0_lst[i].to<flex_list>();
        ASSERT_EQ(dim1_lst.size(), field_length_p[1]);
        for (size_t j = 0; j < dim1_lst.size(); ++j) {
          auto value_type = dim1_lst[j].get_type();
          if (value_type == flex_type_enum::INTEGER ||
              value_type == flex_type_enum::FLOAT) {
            outptr[outstrides[0] * i + outstrides[1] * j] = (float)(dim1_lst[j]);
          } else {
            ASSERT_MSG(false, "Unsupported typed");
          }
        }
      } else {
        ASSERT_MSG(false, "Unsupported typed");
      }
    }
  } else {
    // Case 4: Array type or mixed types
    ASSERT_EQ(outstrides.size(), 1);
    size_t pos = 0;
    for (size_t i = 0; i < data.size(); ++i) {
      auto type = data[i].get_type();
      if (type == flex_type_enum::VECTOR) {
        const flex_vec& v = data[i].to<flex_vec>();
        ASSERT_EQ(v.size(), field_length_p[i]);
        for (size_t j = 0; j < v.size(); ++j) {
          outptr[outstrides[0] * pos] = (float)(v[j]);
          ++pos;
        }
      } else if (type == flex_type_enum::INTEGER ||
                 type == flex_type_enum::FLOAT) {
        outptr[outstrides[0] * pos] = (float)(data[i]);
        ++pos;
      } else {
        ASSERT_MSG(false, "Unsupported type");
      }
    }
  }
  return;
}
void sframe_load_to_numpy(turi::gl_sframe input, size_t outptr_addr,
                     std::vector<size_t> outstrides,
                     std::vector<size_t> field_length,
                     size_t begin, size_t end) {
  if (!input.is_materialized()) {
    input.materialize();
  }

  ASSERT_MSG(input.num_columns() > 0, "SFrame has no column");
  float* outptr = reinterpret_cast<float*>(outptr_addr);

  ASSERT_GE(outstrides.size(), 1);
  for (size_t& stride: outstrides) {
    stride /= sizeof(float);
  }
  // we consume the first index. copy_to_memory takes the rest
  std::vector<size_t> descendent_strides(outstrides.begin() + 1, outstrides.end());
  for (const auto& row : input.range_iterator(begin, end)) {
    copy_to_memory(row, outptr, descendent_strides, field_length);
    outptr += outstrides[0];
  }
}

// Loads image into row-major array with shape HWC (height, width, channel)
void image_load_to_numpy(const image_type& img, size_t outptr_addr,
                         const std::vector<size_t>& outstrides) {
  unsigned char *outptr = reinterpret_cast<unsigned char *>(outptr_addr);
  copy_image_to_memory(img, outptr, outstrides, true);
}


BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(sframe_load_to_numpy, "input", "outptr_addr", "outstrides", "field_length", "begin", "end");
REGISTER_FUNCTION(image_load_to_numpy, "img", "outptr_addr", "outstrides");
END_FUNCTION_REGISTRATION
