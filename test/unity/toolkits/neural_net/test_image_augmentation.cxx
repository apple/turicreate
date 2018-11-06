/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_image_augmentation

#include <unity/toolkits/neural_net/image_augmentation.hpp>

#include <algorithm>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <image/image_type.hpp>
#include <util/test_macros.hpp>

namespace turi {
namespace neural_net {
namespace {

BOOST_AUTO_TEST_CASE(test_image_box_constructor) {
  // Default boxes are zero-initialized.
  image_box box;
  TS_ASSERT_EQUALS(box.x, 0.f);
  TS_ASSERT_EQUALS(box.y, 0.f);
  TS_ASSERT_EQUALS(box.width, 0.f);
  TS_ASSERT_EQUALS(box.height, 0.f);

  // The constructor takes arguments in x,y,width,height order.
  box = image_box(1.f, 2.f, 3.f, 4.f);  
  TS_ASSERT_EQUALS(box.x, 1.f);
  TS_ASSERT_EQUALS(box.y, 2.f);
  TS_ASSERT_EQUALS(box.width, 3.f);
  TS_ASSERT_EQUALS(box.height, 4.f);
}

BOOST_AUTO_TEST_CASE(test_image_box_area) {
  // Typical case
  image_box box(0.f, 0.f, 0.4f, 0.5f);
  TS_ASSERT_EQUALS(box.area(), 0.2f);

  // Any negative width or height yields zero area.
  box = image_box(1.f, 1.f, -0.5f, 0.5f);
  TS_ASSERT_EQUALS(box.area(), 0.f);
  box = image_box(1.f, 1.f, 0.5f, -0.5f);
  TS_ASSERT_EQUALS(box.area(), 0.f);
}

BOOST_AUTO_TEST_CASE(test_image_box_normalize) {
  // Typical case
  image_box box(10.f, 20.f, 30.f, 40.f);
  box.normalize(100.f, 50.f);
  TS_ASSERT_EQUALS(box, image_box(0.1f, 0.4f, 0.3f, 0.8f));
}

BOOST_AUTO_TEST_CASE(test_image_box_clip) {
  image_box box;

  // Clipping to a larger box is a no-op.
  box = image_box(10.f, 20.f, 30.f, 40.f);
  box.clip(image_box(0.f, 0.f, 100.f, 100.f));
  TS_ASSERT_EQUALS(box, image_box(10.f, 20.f, 30.f, 40.f));

  // Clipping to a strictly contained box results in the contained box.
  box = image_box(10.f, 20.f, 30.f, 40.f);
  box.clip(image_box(15.f, 25.f, 10.f, 10.f));
  TS_ASSERT_EQUALS(box, image_box(15.f, 25.f, 10.f, 10.f));

  // Clipping to an overlapping box returns the intersection.
  box = image_box(10.f, 20.f, 30.f, 40.f);
  box.clip(image_box(20.f, 0.f, 10.f, 80.f));
  TS_ASSERT_EQUALS(box, image_box(20.f, 20.f, 10.f, 40.f));

  // Clipping to a non-overlapping box return an empty box.
  // Clipping to a strictly contained box results in the contained box.
  box = image_box(10.f, 20.f, 30.f, 40.f);
  box.clip(image_box(70.f, 70.f, 100.f, 100.f));
  TS_ASSERT_EQUALS(box.area(), 0.f);
}

image_type create_black_image(size_t width, size_t height) {
  std::vector<char> buffer(height*width*3, 0);
  return image_type(buffer.data(), height, width, 3, buffer.size(),
                    IMAGE_TYPE_CURRENT_VERSION,
                    static_cast<int>(Format::RAW_ARRAY));
}

BOOST_AUTO_TEST_CASE(test_resize_only_image_augmenter) {

  // Create some arbitrary-size images.
  std::vector<labeled_image> source_batch(4);
  source_batch[0].image = create_black_image(100, 200);
  source_batch[1].image = create_black_image(200, 100);
  source_batch[2].image = create_black_image(400, 400);
  source_batch[3].image = create_black_image(100, 500);

  // Add some arbitrary annotations.
  source_batch[0].annotations.resize(1);
  source_batch[0].annotations[0].identifier = 1;
  source_batch[0].annotations[0].bounding_box =
      image_box(10.f, 10.f, 20.f, 20.f);
  source_batch[2].annotations.resize(2);
  source_batch[2].annotations[0].identifier = 2;
  source_batch[2].annotations[0].bounding_box =
      image_box(20.f, 20.f, 20.f, 20.f);
  source_batch[2].annotations[1].identifier = 3;
  source_batch[2].annotations[1].bounding_box =
      image_box(30.f, 30.f, 20.f, 20.f);

  // Configure an augmenter to resize to 400x300.
  image_augmenter::options options;
  options.output_width = 400;
  options.output_height = 300;

  // Create the augmenter.
  resize_only_image_augmenter augmenter(options);
  TS_ASSERT_EQUALS(augmenter.get_options().output_width, options.output_width);
  TS_ASSERT_EQUALS(augmenter.get_options().output_height,
                   options.output_height);

  // Invoke the augmenter.
  image_augmenter::result result = augmenter.prepare_images(source_batch);

  // Validate the shape of the float array.
  TS_ASSERT_EQUALS(result.image_batch.dim(), 4);
  TS_ASSERT_EQUALS(result.image_batch.shape()[0], 4);    // N
  TS_ASSERT_EQUALS(result.image_batch.shape()[1], 300);  // H
  TS_ASSERT_EQUALS(result.image_batch.shape()[2], 400);  // W
  TS_ASSERT_EQUALS(result.image_batch.shape()[3], 3);    // C

  // Validate that each image is still black
  const float* data = result.image_batch.data();
  size_t size = result.image_batch.size();
  auto is_zero = [](float x) { return x == 0.f; };
  TS_ASSERT(std::all_of(data, data + size, is_zero));

  // Validate that the annotations were copied.
  for (int i = 0; i < 4; ++i) {
    TS_ASSERT_EQUALS(source_batch[i].annotations, result.annotations_batch[i]);
  }
}

}  // namespace
}  // neural_net
}  // turi
