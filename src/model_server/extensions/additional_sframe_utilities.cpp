/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <vector>
#include <core/parallel/lambda_omp.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <model_server/lib/image_util.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <core/storage/sframe_data/sframe_config.hpp>
#include <core/data/image/image_type.hpp>
#include <core/data/image/io.hpp>
#include "additional_sframe_utilities.hpp"
using namespace turi;


template <typename T>
void copy_image_to_memory(const image_type& input, T *outptr,
                          const std::vector<size_t>& outstrides,
                          const std::vector<size_t>& outshape,
                          bool channel_last) {
  ASSERT_EQ(outstrides.size(), 3);
  ASSERT_EQ(outshape.size(), 3);
  size_t stride_h, stride_w, stride_c;
  size_t height, width, channels;
  if (channel_last) {
    // Format: HWC
    stride_h = outstrides[0];
    stride_w = outstrides[1];
    stride_c = outstrides[2];
    height = outshape[0];
    width = outshape[1];
    channels = outshape[2];
  } else {
    // Format: CHW
    stride_c = outstrides[0];
    stride_h = outstrides[1];
    stride_w = outstrides[2];
    channels = outshape[0];
    height = outshape[1];
    width = outshape[2];
  }

  // Resize.
  flexible_type resized = image_util::resize_image(input, width, height,
						   channels, /* decode */ true);
  const image_type& img = resized.get<flex_image>();

  // Copy.
  size_t cnt = 0;
  const unsigned char* raw_data = img.get_image_data();
  for (size_t i = 0; i < img.m_height; ++i) {
    for (size_t j = 0; j < img.m_width; ++j) {
      for (size_t k = 0; k < img.m_channels; ++k) {
	outptr[i * stride_h + j * stride_w + k * stride_c] =
	    static_cast<T>(raw_data[cnt++]);
      }
    }
  }

  // Further optimization is possible (but not trivial) by combining the resize
  // operation and the copy operation, removing an intermediate buffer.
}

void copy_to_memory(const sframe_rows::row& data,
                    float* outptr,
                    const std::vector<size_t>& outstrides,
                    const std::vector<size_t>& outshape) {
  ASSERT_GE(data.size(), 1);

  for (size_t i = 0; i < data.size(); ++i) {
    ASSERT_NE((int)data[i].get_type(), (int)flex_type_enum::UNDEFINED);
  }

  // Case 1: Image type
  auto type = data[0].get_type();
  if (type == flex_type_enum::IMAGE) {
    ASSERT_MSG(data.size() == 1, "Image data only support one input field");
    const image_type& img = data[0].get<flex_image>();
    copy_image_to_memory<float>(img, outptr, outstrides, outshape, false);
    return;
  } else if (data.size() == 1 && (type == flex_type_enum::FLOAT || type == flex_type_enum::INTEGER)) {
    // Case 2: Single value type (should really get rid of this special case)
    ASSERT_EQ(outstrides.size(), 0);
    (*outptr) = (float)(data[0]);
    return;
  } else if (data.size() == 1 && type == flex_type_enum::LIST) {
    // Case 3: 2D arrays: list of vectors or list of lists of values
    ASSERT_EQ(outshape.size(), 2);
    const flex_list& dim0_lst = data[0].to<flex_list>();
    ASSERT_EQ(dim0_lst.size(), outshape[0]);
    for (size_t i = 0; i < dim0_lst.size(); ++i) {
      auto dim1_type = dim0_lst[i].get_type();
      if (dim1_type == flex_type_enum::VECTOR) {
        const flex_vec& dim1_vec = dim0_lst[i].to<flex_vec>();
        ASSERT_EQ(dim1_vec.size(), outshape[1]);
        for (size_t j = 0; j < dim1_vec.size(); ++j) {
          outptr[outstrides[0] * i + outstrides[1] * j] = (float)(dim1_vec[j]);
        }
      } else if (dim1_type == flex_type_enum::LIST) {
        const flex_list& dim1_lst = dim0_lst[i].to<flex_list>();
        ASSERT_EQ(dim1_lst.size(), outshape[1]);
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
    ASSERT_EQ(outshape.size(), 1);
    size_t pos = 0;
    for (size_t i = 0; i < data.size(); ++i) {
      auto type = data[i].get_type();
      if (type == flex_type_enum::VECTOR) {
        const flex_vec& v = data[i].to<flex_vec>();
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
    ASSERT_EQ(pos, outshape[0]);
  }
  return;
}
void sframe_load_to_numpy(turi::gl_sframe input, size_t outptr_addr,
			  std::vector<size_t> outstrides,
			  std::vector<size_t> outshape,
			  size_t begin, size_t end) {
  if (!input.is_materialized()) {
    input.materialize();
  }

  ASSERT_MSG(input.num_columns() > 0, "SFrame has no column");
  float* outptr = reinterpret_cast<float*>(outptr_addr);

  ASSERT_EQ(outstrides.size(), outshape.size());
  ASSERT_GE(outstrides.size(), 1);
  for (size_t& stride: outstrides) {
    stride /= sizeof(float);
  }

  // we consume the first index. copy_to_memory takes the rest
  size_t row_stride = outstrides[0];
  outstrides.erase(outstrides.begin());
  outshape.erase(outshape.begin());

  const size_t num_rows = end - begin;
  in_parallel([&](size_t worker_idx, size_t num_workers) {
    // Compute the input range and output address for this thread.
    size_t worker_begin = begin + num_rows * worker_idx / num_workers;
    size_t worker_end = begin + num_rows * (worker_idx + 1) / num_workers;
    float* worker_out = outptr + row_stride * (worker_begin - begin);

    for (const auto& row : input.range_iterator(worker_begin, worker_end)) {
      copy_to_memory(row, worker_out, outstrides, outshape);
      worker_out += row_stride;
    }
  });
}

// Loads image into row-major array with shape HWC (height, width, channel)
void image_load_to_numpy(const image_type& img, size_t outptr_addr,
                         const std::vector<size_t>& outstrides) {
  unsigned char *outptr = reinterpret_cast<unsigned char *>(outptr_addr);
  copy_image_to_memory(img, outptr, outstrides,
                       {img.m_height, img.m_width, img.m_channels}, true);
}


BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(sframe_load_to_numpy, "input", "outptr_addr", "outstrides", "outshape", "begin", "end");
REGISTER_FUNCTION(image_load_to_numpy, "img", "outptr_addr", "outstrides");
END_FUNCTION_REGISTRATION
