/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <vector>
#include <core/parallel/lambda_omp.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <model_server/lib/image_util.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <core/storage/sframe_data/sframe_config.hpp>
#include <core/data/image/image_type.hpp>
#include <core/data/image/io.hpp>
#include "additional_sframe_utilities.hpp"

using namespace turi;

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
