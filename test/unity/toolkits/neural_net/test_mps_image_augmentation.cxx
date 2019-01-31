/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_mps_image_augmentation

#include <unity/toolkits/neural_net/mps_compute_context.hpp>

#include <tuple>

#include <boost/test/unit_test.hpp>
#include <image/image_type.hpp>
#include <unity/lib/image_util.hpp>
#include <util/test_macros.hpp>

namespace turi {
namespace neural_net {
namespace {

using rgb_pixel_type = std::tuple<uint8_t,uint8_t,uint8_t>;
using shape_type = std::vector<size_t>;
using rng_impl = std::function<float(float lower, float upper)>;

constexpr int kClassIdentifier = 7;
constexpr float kObjectConfidence = 1.f;
constexpr float kEpsilon = 0.002f;

image_type create_image(
    size_t width, size_t height,
    std::function<rgb_pixel_type(size_t x, size_t y)> rgb_generator) {

  size_t size = height * width * 3;
  std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      uint8_t* pixel = buffer.get() + j * width * 3 + i * 3;
      std::tie(pixel[0], pixel[1], pixel[2]) = rgb_generator(i, j);
    }
  }

  flex_image raw_image(reinterpret_cast<char*>(buffer.get()), height, width, 3,
		       size, IMAGE_TYPE_CURRENT_VERSION,
		       static_cast<int>(Format::RAW_ARRAY));
  return image_util::encode_image(raw_image);
}

std::vector<image_annotation> create_annotations(std::vector<image_box> boxes) {

  std::vector<image_annotation> result(boxes.size());
  for (size_t i = 0; i < boxes.size(); ++i) {
    result[i].identifier = kClassIdentifier;
    result[i].bounding_box = boxes[i];
    result[i].confidence = kObjectConfidence;
  }
  return result;
}

shape_type get_shape(const float_array& arr) {
  return shape_type(arr.shape(), arr.shape() + arr.dim());
}

// Return a random number generator that dispatches each call to a provided
// sequence of implementations. Each of these one-off implementations is
// expected to make assertions about the inputs and to provide a mock output.
rng_impl create_mock_rng(std::deque<rng_impl>* expected_calls) {
  return [expected_calls](float lower, float upper) {
    TS_ASSERT(!expected_calls->empty());
    rng_impl expected_call = std::move(expected_calls->front());
    expected_calls->pop_front();
    return expected_call(lower, upper);
  };
}

BOOST_AUTO_TEST_CASE(test_resize) {

  // Create an augmenter that just resizes to 512 by 512.
  image_augmenter::options opts;
  opts.batch_size = 1;
  opts.output_width = 512;
  opts.output_height = 512;
  auto augmenter = mps_compute_context().create_image_augmenter(opts);

  std::vector<labeled_image> batch(1);

  // Create a single 256 by 256 image, encoding the column index into the red
  // channel, the row index into the green channel, and leaving blue at 0.
  auto generator = [](size_t x, size_t y) {
    return rgb_pixel_type(static_cast<uint8_t>(x), static_cast<uint8_t>(y), 0);
  };
  batch[0].image = create_image(256, 256, generator);

  // Create a single annotation, at the upper-left quadrant.
  batch[0].annotations =
      create_annotations({ image_box(0.f, 0.f, 0.5f, 0.5f) });

  // Apply the resize augmentation.
  image_augmenter::result res = augmenter->prepare_images(batch);

  // The result should contain one 512 by 512 image.
  TS_ASSERT_EQUALS(get_shape(res.image_batch), shape_type({1, 512, 512, 3}));
  shared_float_array res_image = res.image_batch[0];

  // The upper-left corner should be black.
  TS_ASSERT_EQUALS(res_image[0][0].data()[0], 0.f);  // R
  TS_ASSERT_EQUALS(res_image[0][0].data()[1], 0.f);  // G
  TS_ASSERT_EQUALS(res_image[0][0].data()[2], 0.f);  // B

  // The upper-right corner should be red.
  TS_ASSERT_EQUALS(res_image[0][511].data()[0], 1.f);  // R
  TS_ASSERT_EQUALS(res_image[0][511].data()[1], 0.f);  // G
  TS_ASSERT_EQUALS(res_image[0][511].data()[2], 0.f);  // B

  // The lower-left corner should be green.
  TS_ASSERT_EQUALS(res_image[511][0].data()[0], 0.f);  // R
  TS_ASSERT_EQUALS(res_image[511][0].data()[1], 1.f);  // G
  TS_ASSERT_EQUALS(res_image[511][0].data()[2], 0.f);  // B

  // The lower-right corner should be all yellow.
  TS_ASSERT_EQUALS(res_image[511][511].data()[0], 1.f);  // R
  TS_ASSERT_EQUALS(res_image[511][511].data()[1], 1.f);  // G
  TS_ASSERT_EQUALS(res_image[511][511].data()[2], 0.f);  // B

  // The annotation should not have changed.
  TS_ASSERT_EQUALS(res.annotations_batch.size(), 1);
  const std::vector<image_annotation>& annotations = res.annotations_batch[0];
  TS_ASSERT_EQUALS(annotations.size(), 1);
  image_annotation annotation = annotations[0];
  TS_ASSERT_EQUALS(annotation.identifier, kClassIdentifier);
  TS_ASSERT_EQUALS(annotation.bounding_box, image_box(0.f, 0.f, 0.5f, 0.5f));
  TS_ASSERT_EQUALS(annotation.confidence, kObjectConfidence);
}

BOOST_AUTO_TEST_CASE(test_horizontal_flip) {

  std::deque<rng_impl> rng_calls;

  // Create an augmenter that just performs horizontal flip.
  image_augmenter::options opts;
  opts.batch_size = 1;
  opts.output_width = 256;
  opts.output_height = 256;
  opts.horizontal_flip_prob = 0.5;
  auto augmenter = mps_compute_context().create_image_augmenter_for_testing(
      opts, create_mock_rng(&rng_calls));

  std::vector<labeled_image> batch(1);

  // Create a single 256 by 256 image, encoding the column index into the red
  // channel, the row index into the green channel, and leaving blue at 0.
  auto generator = [](size_t x, size_t y) {
    return rgb_pixel_type(static_cast<uint8_t>(x), static_cast<uint8_t>(y), 0);
  };
  batch[0].image = create_image(256, 256, generator);

  // Create a single annotation, at the upper-left quadrant.
  batch[0].annotations =
      create_annotations({ image_box(0.f, 0.f, 0.5f, 0.5f) });

  // Rig the RNG to cause the image to be flipped.
  auto skip_draw = [](float lower, float upper) {
    TS_ASSERT_EQUALS(lower, 0.f);
    TS_ASSERT_EQUALS(upper, 1.f);
    return 0.75f;  // Greater than skip threshold of 0.5f
  };
  rng_calls.push_back(skip_draw);

  // Apply the augmentation.
  image_augmenter::result res = augmenter->prepare_images(batch);
  TS_ASSERT(rng_calls.empty());  // All expected calls to RNG were made

  // The result should contain one 256 by 256 image.
  TS_ASSERT_EQUALS(get_shape(res.image_batch), shape_type({1, 256, 256, 3}));
  shared_float_array res_image = res.image_batch[0];

  // The upper-left corner should be red.
  TS_ASSERT_EQUALS(res_image[0][0].data()[0], 1.f);  // R
  TS_ASSERT_EQUALS(res_image[0][0].data()[1], 0.f);  // G
  TS_ASSERT_EQUALS(res_image[0][0].data()[2], 0.f);  // B

  // The upper-right corner should be black.
  TS_ASSERT_EQUALS(res_image[0][255].data()[0], 0.f);  // R
  TS_ASSERT_EQUALS(res_image[0][255].data()[1], 0.f);  // G
  TS_ASSERT_EQUALS(res_image[0][255].data()[2], 0.f);  // B

  // The lower-left corner should be yellow.
  TS_ASSERT_EQUALS(res_image[255][0].data()[0], 1.f);  // R
  TS_ASSERT_EQUALS(res_image[255][0].data()[1], 1.f);  // G
  TS_ASSERT_EQUALS(res_image[255][0].data()[2], 0.f);  // B

  // The lower-right corner should be green.
  TS_ASSERT_EQUALS(res_image[255][255].data()[0], 0.f);  // R
  TS_ASSERT_EQUALS(res_image[255][255].data()[1], 1.f);  // G
  TS_ASSERT_EQUALS(res_image[255][255].data()[2], 0.f);  // B

  // The one annotated object should now be in the upper right quadrant.
  TS_ASSERT_EQUALS(res.annotations_batch.size(), 1);
  const std::vector<image_annotation>& annotations = res.annotations_batch[0];
  TS_ASSERT_EQUALS(annotations.size(), 1);
  image_annotation annotation = annotations[0];
  TS_ASSERT_EQUALS(annotation.identifier, kClassIdentifier);
  TS_ASSERT_EQUALS(annotation.bounding_box, image_box(0.5f, 0.f, 0.5f, 0.5f));
  TS_ASSERT_EQUALS(annotation.confidence, kObjectConfidence);
}

BOOST_AUTO_TEST_CASE(test_crop) {

  std::deque<rng_impl> rng_calls;

  // Create an augmenter that just performs crops.
  image_augmenter::options opts;
  opts.batch_size = 1;
  opts.output_width = 256;
  opts.output_height = 256;
  opts.crop_prob = 0.5f;
  opts.crop_opts.min_aspect_ratio = 0.5f;
  opts.crop_opts.max_aspect_ratio = 2.f;
  opts.crop_opts.min_area_fraction = 0.125f;
  opts.crop_opts.max_area_fraction = 1.f;
  opts.crop_opts.min_object_covered = 0.f;
  opts.crop_opts.max_attempts = 2;
  opts.crop_opts.min_eject_coverage = 0.5f;
  auto augmenter = mps_compute_context().create_image_augmenter_for_testing(
      opts, create_mock_rng(&rng_calls));

  std::vector<labeled_image> batch(1);

  // Create a single 256 by 256 image, encoding the column index into the red
  // channel, the row index into the green channel, and leaving blue at 0.
  auto generator = [](size_t x, size_t y) {
    return rgb_pixel_type(static_cast<uint8_t>(x), static_cast<uint8_t>(y), 0);
  };
  batch[0].image = create_image(256, 256, generator);

  // Create two annotations. The first covers the lower three quarters of the
  // left half of the image. The second covers the entier right half of the
  // image.
  batch[0].annotations =
      create_annotations({ image_box(0.0f, 0.25f, 0.5f, 0.75f),
                           image_box(0.5f, 0.00f, 0.5f, 1.00f)  });

  // Pre-program the behavior of the RNG.
  auto skip_draw = [](float lower, float upper) {
    TS_ASSERT_EQUALS(lower, 0.f);
    TS_ASSERT_EQUALS(upper, 1.f);
    return 0.75f;  // Greater than skip threshold of 0.5f
  };
  rng_calls.push_back(skip_draw);
  auto aspect_ratio_draw = [](float lower, float upper) {
    TS_ASSERT_EQUALS(lower, 0.5f);  // min_aspect_ratio
    TS_ASSERT_EQUALS(upper, 2.0f);  // max_aspect_ratio
    return 2.f;  // Specify a crop twice as wide as it is tall (2:1)
  };
  rng_calls.push_back(aspect_ratio_draw);
  auto height_draw = [](float lower, float upper) {
    // min_area_fraction 0.125 and aspect_ratio 2 implies a width fraction of
    // 0.5 and a height fraction of 0.25.
    TS_ASSERT_EQUALS(lower,  64.f);  // 256 * 0.25
    TS_ASSERT_EQUALS(upper, 128.f);  // 256 * 0.5
    return 128.f;  // Specify a full width crop (implying half height)
  };
  rng_calls.push_back(height_draw);
  auto x_draw = [](float lower, float upper) {
    // Full width crop leaves no uncertainty in horizontal position
    TS_ASSERT_EQUALS(lower, 0.f);
    TS_ASSERT_EQUALS(upper, 0.f);
    return 0.f;
  };
  rng_calls.push_back(x_draw);
  auto y_draw = [](float lower, float upper) {
    TS_ASSERT_EQUALS(lower, 0.f);
    TS_ASSERT_EQUALS(upper, 128.f);
    return 128.f;  // Leave the crop at the top (in Core Image coordinates)
  };
  rng_calls.push_back(y_draw);

  // Apply the augmentation.
  image_augmenter::result res = augmenter->prepare_images(batch);
  TS_ASSERT(rng_calls.empty());  // All expected calls to RNG were made

  // The result should contain one 256 by 256 image, a scaled version of the
  // top half of the original image.
  TS_ASSERT_EQUALS(get_shape(res.image_batch), shape_type({1, 256, 256, 3}));
  shared_float_array res_image = res.image_batch[0];

  // The upper-left corner should be black.
  TS_ASSERT_DELTA(res_image[0][0].data()[0], 0.f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[0][0].data()[1], 0.f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[0][0].data()[2], 0.f, kEpsilon);  // B

  // The upper-right corner should be red.
  TS_ASSERT_DELTA(res_image[0][255].data()[0], 1.f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[0][255].data()[1], 0.f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[0][255].data()[2], 0.f, kEpsilon);  // B

  // The lower-left corner should be half-intensity green.
  TS_ASSERT_DELTA(res_image[255][0].data()[0], 0.0f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[255][0].data()[1], 0.5f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[255][0].data()[2], 0.0f, kEpsilon);  // B

  // The lower-right corner should be full red plus half green.
  TS_ASSERT_DELTA(res_image[255][255].data()[0], 1.0f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[255][255].data()[1], 0.5f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[255][255].data()[2], 0.0f, kEpsilon);  // B

  // The smaller (first) annotation should have been discarded, since only one
  // third of its area was preserved by the crop. The second annotation should
  // still cover the entire right half of the image.
  TS_ASSERT_EQUALS(res.annotations_batch.size(), 1);
  const std::vector<image_annotation>& annotations = res.annotations_batch[0];
  TS_ASSERT_EQUALS(annotations.size(), 1);
  image_annotation annotation = annotations[0];
  TS_ASSERT_EQUALS(annotation.identifier, kClassIdentifier);
  TS_ASSERT_EQUALS(annotation.bounding_box, image_box(0.5f, 0.f, 0.5f, 1.f));
  TS_ASSERT_EQUALS(annotation.confidence, kObjectConfidence);
}

BOOST_AUTO_TEST_CASE(test_pad) {

  std::deque<rng_impl> rng_calls;

  // Create an augmenter that just performs padding
  image_augmenter::options opts;
  opts.batch_size = 1;
  opts.output_width = 256;
  opts.output_height = 256;
  opts.pad_prob = 0.5f;
  opts.pad_opts.min_aspect_ratio = 0.5f;
  opts.pad_opts.max_aspect_ratio = 2.f;
  opts.pad_opts.min_area_fraction = 1.f;
  opts.pad_opts.max_area_fraction = 4.f;
  opts.pad_opts.max_attempts = 2;
  auto augmenter = mps_compute_context().create_image_augmenter_for_testing(
      opts, create_mock_rng(&rng_calls));

  std::vector<labeled_image> batch(1);

  // Create a single 256 by 256 image, encoding the column index into the red
  // channel, the row index into the green channel, and leaving blue at 0.
  auto generator = [](size_t x, size_t y) {
    return rgb_pixel_type(static_cast<uint8_t>(x), static_cast<uint8_t>(y), 0);
  };
  batch[0].image = create_image(256, 256, generator);

  // Create a single annotation, at the upper-left quadrant.
  batch[0].annotations =
      create_annotations({ image_box(0.f, 0.f, 0.5f, 0.5f) });

  // Pre-program the behavior of the RNG.
  auto skip_draw = [](float lower, float upper) {
    TS_ASSERT_EQUALS(lower, 0.f);
    TS_ASSERT_EQUALS(upper, 1.f);
    return 0.75f;  // Greater than skip threshold of 0.5f
  };
  rng_calls.push_back(skip_draw);
  auto aspect_ratio_draw = [](float lower, float upper) {
    TS_ASSERT_EQUALS(lower, 0.5f);  // min_aspect_ratio
    TS_ASSERT_EQUALS(upper, 2.0f);  // max_aspect_ratio
    return 1.f;  // Preserve aspect ratio
  };
  rng_calls.push_back(aspect_ratio_draw);
  auto height_draw = [](float lower, float upper) {
    // max_area_fraction 4 and aspect_ratio 1:1 allows for a 2x linear increase
    // in final size
    TS_ASSERT_EQUALS(lower, 256.f);
    TS_ASSERT_EQUALS(upper, 512.f);
    return 512.f;  // The padded image should be twice as tall.
  };
  rng_calls.push_back(height_draw);
  auto x_draw = [](float lower, float upper) {
    TS_ASSERT_EQUALS(lower, 0.f);
    TS_ASSERT_EQUALS(upper, 256.f);
    return 256.f;  // The source image should be on the right edge.
  };
  rng_calls.push_back(x_draw);
  auto y_draw = [](float lower, float upper) {
    TS_ASSERT_EQUALS(lower, 0.f);
    TS_ASSERT_EQUALS(upper, 256.f);
    return 0.f;  // Leave the source at the bottom (in Core Image coordinates)
  };
  rng_calls.push_back(y_draw);

  // Apply the augmentation.
  image_augmenter::result res = augmenter->prepare_images(batch);
  TS_ASSERT(rng_calls.empty());  // All expected calls to RNG were made

  // The result should contain one 256 by 256 image, a scaled version of the
  // padded 512 by 512 image. The result should have the original image in the
  // lower-right corner (from coordinates (128, 128) to (255, 255))
  TS_ASSERT_EQUALS(get_shape(res.image_batch), shape_type({1, 256, 256, 3}));
  shared_float_array res_image = res.image_batch[0];

  // The upper-left corner should be gray.
  TS_ASSERT_DELTA(res_image[0][0].data()[0], 0.5f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[0][0].data()[1], 0.5f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[0][0].data()[2], 0.5f, kEpsilon);  // B

  // The upper-right corner should be gray.
  TS_ASSERT_DELTA(res_image[0][255].data()[0], 0.5f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[0][255].data()[1], 0.5f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[0][255].data()[2], 0.5f, kEpsilon);  // B

  // The lower-left corner should be gray.
  TS_ASSERT_DELTA(res_image[255][0].data()[0], 0.5f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[255][0].data()[1], 0.5f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[255][0].data()[2], 0.5f, kEpsilon);  // B

  // The lower-right corner should be yellow. This is also the lower-right
  // corner of the original image.
  TS_ASSERT_DELTA(res_image[255][255].data()[0], 1.f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[255][255].data()[1], 1.f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[255][255].data()[2], 0.f, kEpsilon);  // B

  // The upper-left corner of the original image should still be black.
  TS_ASSERT_DELTA(res_image[128][128].data()[0], 0.f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[128][128].data()[1], 0.f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[128][128].data()[2], 0.f, kEpsilon);  // B
  
  // The upper-right corner of the original image should still be red.
  TS_ASSERT_DELTA(res_image[128][255].data()[0], 1.f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[128][255].data()[1], 0.f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[128][255].data()[2], 0.f, kEpsilon);  // B

  // The lower-left corner of the original image should still be green.
  TS_ASSERT_DELTA(res_image[255][128].data()[0], 0.f, kEpsilon);  // R
  TS_ASSERT_DELTA(res_image[255][128].data()[1], 1.f, kEpsilon);  // G
  TS_ASSERT_DELTA(res_image[255][128].data()[2], 0.f, kEpsilon);  // B

  // The annotation which was the upper-left quadrant of the original image is
  // now the upper-left quadrant of the lower-right quadrant.
  TS_ASSERT_EQUALS(res.annotations_batch.size(), 1);
  const std::vector<image_annotation>& annotations = res.annotations_batch[0];
  TS_ASSERT_EQUALS(annotations.size(), 1);
  image_annotation annotation = annotations[0];
  TS_ASSERT_EQUALS(annotation.identifier, kClassIdentifier);
  TS_ASSERT_EQUALS(annotation.bounding_box,
                   image_box(0.5f, 0.5f, 0.25f, 0.25f));
  TS_ASSERT_EQUALS(annotation.confidence, kObjectConfidence);
}

}  // namespace
}  // neural_net
}  // turi
