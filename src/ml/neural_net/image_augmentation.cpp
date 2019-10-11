/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/image_augmentation.hpp>

#include <algorithm>

#include <core/logging/assertions.hpp>
#include <model_server/lib/image_util.hpp>

namespace turi {
namespace neural_net {

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

image_augmenter::result resize_only_image_augmenter::prepare_images(
    std::vector<labeled_image> source_batch) {

  const size_t n = opts_.batch_size;
  const size_t h = opts_.output_height;
  const size_t w = opts_.output_width;
  constexpr size_t c = 3;

  result res;
  res.annotations_batch.reserve(n);

  // Discard any source data in excess of the batch size.
  if (source_batch.size() > n) {
    source_batch.resize(n);
  }

  // Allocate a float vector large enough to contain the entire image batch.
  std::vector<float> result_array(n * h * w * c);

  // Note: this computation could probably be parallelized, if needed.
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

image_augmenter::result processed_image_augmenter::prepare_images(
  std::vector<labeled_image> source_batch) {

  result res;

  const size_t n = opts_.batch_size;
  constexpr size_t c = 3;

  std::vector<turi::neural_net::shared_float_array> images_to_aug;
  std::vector<turi::neural_net::shared_float_array> ann_batch;
  std::vector<turi::neural_net::shared_float_array> pred_batch;

  // Decode a batch of images to raw format 
  for (size_t i = 0; i < n; i++) {

    size_t input_height = source_batch[i].image.m_height;
    size_t input_width = source_batch[i].image.m_width;
    std::vector<float> img( input_height * input_width * c, 0.f);
    std::cout << "0";
    // unsigned char *outptr = reinterpret_cast<unsigned char *>(img.data());
    image_util::copy_image_to_memory(source_batch[i].image , img.data(),
        {input_width * c , c , 1},
        {input_height, input_width, c}, true);
    std::cout<<"1";

    std::transform(img.begin(), img.end(), img.begin(), [](float pixel) -> float { return pixel/255; });
    std::cout<<"2";
    shared_float_array image_to_aug = shared_float_array::wrap(img, {input_height, input_width, c});
    std::cout<<"3";
    images_to_aug.push_back(image_to_aug);
    std::cout<<"4";
    std::vector<float> annotation(source_batch[i].annotations.size() * 6);
    std::cout<<"5";
    size_t x = 0;
    for (size_t j=0; j<source_batch[i].annotations.size(); j++) {
      annotation[x] = source_batch[i].annotations[j].identifier;
      annotation[x+1] = source_batch[i].annotations[j].bounding_box.x;
      annotation[x+2] = source_batch[i].annotations[j].bounding_box.y;
      annotation[x+3] = source_batch[i].annotations[j].bounding_box.height;
      annotation[x+4] = source_batch[i].annotations[j].bounding_box.width;
      annotation[x+5] = source_batch[i].annotations[j].confidence;
      x = (j + 1) * 6;
    }
    std::cout<<"6";

    shared_float_array ann_to_aug = shared_float_array::wrap(annotation, {source_batch[i].annotations.size() * 6});
    std::cout<<"7";
    ann_batch.push_back(ann_to_aug);

    std::cout<<"8";
    std::vector<float> predictions(source_batch[i].predictions.size() * 6);
    std::cout<<"9";
    size_t y = 0;
    for (size_t k=0; k<source_batch[i].predictions.size(); k++) {
      
      predictions[y] = source_batch[i].predictions[k].identifier;
      predictions[y+1] = source_batch[i].predictions[k].bounding_box.x;
      predictions[y+2] = source_batch[i].predictions[k].bounding_box.y;
      predictions[y+3] = source_batch[i].predictions[k].bounding_box.height;
      predictions[y+4] = source_batch[i].predictions[k].bounding_box.width;
      predictions[y+5] = source_batch[i].predictions[k].confidence;
      y = (k + 1) * 6;
    }
    std::cout<<"10";
    turi::neural_net::shared_float_array pred_to_aug = shared_float_array::wrap(predictions, {source_batch[i].predictions.size() * 6});
    std::cout<<"11";
    pred_batch.push_back(pred_to_aug);
    std::cout<<"12";

  }
  std::cout<<"13";
  
  intermediate_labeled_image input_to_tf_aug;
  input_to_tf_aug.images = images_to_aug;
  input_to_tf_aug.annotations = ann_batch;
  input_to_tf_aug.predictions = pred_batch;
  std::cout<<"14";
  intermediate_result augmented_data = prepare_augmented_images(input_to_tf_aug);
  std::cout<<"24";
  res.image_batch = augmented_data.images;
  std::cout<<"25";
  std::vector<std::vector<image_annotation>> annotations_per_batch;
  for (size_t a= 0; a < n; a++) { 
    std::vector<image_annotation> annotations_per_image;
    std::cout<<"26";
    // const size_t *sh = augmented_data.annotations[a].shape();
    for ( size_t b=0; b < 2; b++ ) {
      std::cout<<"27";
      std::cout << augmented_data.annotations[a][b];
      image_annotation annotation;
      image_box bbox; 
      std::cout << a << b;
      const float *ptr = augmented_data.annotations[a][b].data();
      // int id = *((int *) (ptr+0));
      // annotation.identxifier = id;
      // std::cout<< annotation.identifier;
      std::cout<<"28";
      bbox.x = static_cast<float>(ptr[1]);
      std::cout<<"29";
      bbox.y = static_cast<float>(*augmented_data.annotations[a][b][2].data());
      std::cout<<"30";
      bbox.height = static_cast<float>(*augmented_data.annotations[a][b][3].data());
      std::cout<<"31";
      bbox.width = static_cast<float>(*augmented_data.annotations[a][b][4].data());
      annotation.bounding_box = bbox;
      std::cout<<"32";
      annotation.confidence = static_cast<float>(*augmented_data.annotations[a][b][5].data());
      std::cout<<"33";
      annotations_per_image.push_back(annotation);
   }
   std::cout<<"34";
   annotations_per_batch.push_back(annotations_per_image);

  }
  std::cout<<"35";
  res.annotations_batch = annotations_per_batch;
  return res;
}

}  // neural_net
}  // turi
