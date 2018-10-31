/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/neural_net/image_augmentation.hpp>

#include <algorithm>

#include <logger/assertions.hpp>
#include <unity/lib/image_util.hpp>

namespace turi {
namespace neural_net {

void image_box::scale(float image_width, float image_height) {
  x /= image_width;
  width /= image_width;

  y /= image_height;
  height /= image_height;
}

void image_box::clip(image_box clip_box) {
  float x_max = std::min(x + width, clip_box.x + clip_box.width);
  x = std::max(x, clip_box.x);
  width = x_max - x;

  float y_max = std::min(y + height, clip_box.y + clip_box.height);
  y = std::max(y, clip_box.y);
  height = y_max - y;
}

// static
std::unique_ptr<image_augmenter> image_augmenter::create(const options& opts) {
  // TODO: Implement proper data augmentation.
  std::unique_ptr<image_augmenter> result;
  result.reset(new resize_only_image_augmenter(opts));
  return result;
}

image_augmenter::result resize_only_image_augmenter::prepare_images(
    std::vector<labeled_image> source_batch) {

  const size_t n = source_batch.size();
  const size_t h = opts_.output_height;
  const size_t w = opts_.output_width;
  constexpr size_t c = 3;

  result res;
  res.annotations_batch.reserve(n);

  // Allocate a float vector large enough to contain the entire image batch.
  std::vector<float> result_array(n * h * w * c);

  // TODO: Parallelize this computation, which is currently the bottleneck in
  // training!
  auto out_it = result_array.begin();
  for (labeled_image& source : source_batch) {
    // Resize the input image.
    image_type resized_image = image_util::resize_image(
        source.image, w, h, c, /* decode */ true, /* resample_method */ 1);
    ASSERT_EQ(resized_image.m_image_data_size, h * w * c);

    // Copy the resized image into the output buffer, converting to float
    // (normalized to 1).
    const unsigned char* src_ptr = resized_image.get_image_data();
    auto out_end = out_it + h * w * c;
    while (out_it != out_end) {
      *out_it = *src_ptr / 255.f;
      ++src_ptr;
      ++out_it;
    }

    // Just move the annotations from the input to the output. Since the
    // annotations are all in normalized (relative) coordinates, no modification
    // is required.
    res.annotations_batch.push_back(std::move(source.annotations));
  }

  res.image_batch = shared_float_array::wrap(std::move(result_array),
                                             { n, h, w, c});
  return res;
}

}  // neural_net
}  // turi
