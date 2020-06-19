/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/image_augmentation.hpp>

#include <algorithm>

namespace turi {
namespace neural_net {

shared_float_array convert_to_shared_float_array(
    std::vector<image_annotation> annotations_per_image) {
  std::vector<float> ann(annotations_per_image.size() * 6);
  for (size_t j = 0; j < annotations_per_image.size(); j++) {
    size_t offset = j * 6;
    const image_annotation& annotation = annotations_per_image[j];
    ann[offset] = annotation.identifier;
    ann[offset + 1] = annotation.bounding_box.x;
    ann[offset + 2] = annotation.bounding_box.y;
    ann[offset + 3] = annotation.bounding_box.width;
    ann[offset + 4] = annotation.bounding_box.height;
    ann[offset + 5] = annotation.confidence;
  }
  shared_float_array data_to_augment =
      shared_float_array::wrap(ann, {annotations_per_image.size(), 6});

  return data_to_augment;
}

std::vector<image_annotation> convert_to_image_annotation(
    shared_float_array augmented_annotation) {
  // Check if the annotation is empty (a default value).
  if (augmented_annotation.dim() == 0) {
    return {};
  }

  const size_t* shape = augmented_annotation.shape();
  size_t num_annotations_per_image = shape[0];
  std::vector<image_annotation> augmented_ann(num_annotations_per_image);
  for (size_t b = 0; b < num_annotations_per_image; b++) {
    image_annotation& annotation = augmented_ann[b];
    const float* ptr = augmented_annotation[b].data();
    annotation.identifier = static_cast<int>(ptr[0]);
    annotation.bounding_box.x = ptr[1];
    annotation.bounding_box.y = ptr[2];
    annotation.bounding_box.width = ptr[3];
    annotation.bounding_box.height = ptr[4];
    annotation.confidence = ptr[5];
  }
  return augmented_ann;
}

void image_box::normalize(float image_width, float image_height) {
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

void image_box::extend(const image_box& other) {

  if (other.empty()) return;

  if (empty()) {

    *this = other;

  } else {

    float x_max = std::max(x + width, other.x + other.width);
    x = std::min(x, other.x);
    width = x_max - x;

    float y_max = std::max(y + height, other.y + other.height);
    y = std::min(y, other.y);
    height = y_max - y;
  }
}

bool operator==(const image_box& a, const image_box& b) {
  return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

std::ostream& operator<<(std::ostream& out, const image_box& box) {
  return out << "(x=" << box.x
             << ",y=" << box.y
             << ",w=" << box.width
             << ",h=" << box.height
             << ")";
}

bool operator==(const image_annotation& a, const image_annotation& b) {
  return a.identifier == b.identifier && a.bounding_box == b.bounding_box &&
    a.confidence == b.confidence;
}

image_augmenter::result float_array_image_augmenter::prepare_images(
    std::vector<labeled_image> source_batch) {
  const size_t n = opts_.batch_size;
  const size_t h = opts_.output_height;
  const size_t w = opts_.output_width;
  constexpr size_t c = 3;
  labeled_float_image input_to_tf_aug;
  result res;

  // Discard any source data in excess of the batch size.
  if (source_batch.size() > n) {
    source_batch.resize(n);
  }

  res.annotations_batch.resize(source_batch.size());

  // Decode a batch of images to raw format (shared_float_arrays)
  // Also convert annotations and predictions per batch of images
  // to vectors of shared_float_arrays.
  for (const labeled_image& source : source_batch) {
    size_t input_height = source.image->Height();
    size_t input_width = source.image->Width();
    std::vector<float> img(input_height * input_width * c, 0.f);

    // Decode each image to raw format
    source.image->WriteHWC(MakeSpan(img));

    input_to_tf_aug.images.push_back(
        shared_float_array::wrap(img, {input_height, input_width, c}));
    input_to_tf_aug.annotations.push_back(
        convert_to_shared_float_array(source.annotations));
  }

  // Call the virtual function to use the intermediate data structure and
  // process it
  float_array_result augmented_data = prepare_augmented_images(input_to_tf_aug);

  // Convert augmented_data to the data structure needed
  std::vector<float> result_array(n * h * w * c);
  size_t image_size = augmented_data.images.size();
  const float* start_address = augmented_data.images.data();
  const float* end_address = start_address + image_size;
  std::copy(start_address, end_address, result_array.begin());
  res.image_batch =
      shared_float_array::wrap(std::move(result_array), {n, h, w, c});

  for (size_t a = 0; a < source_batch.size(); a++) {
    res.annotations_batch[a] =
        convert_to_image_annotation(augmented_data.annotations[a]);
  }

  return res;
}

}  // neural_net
}  // turi
