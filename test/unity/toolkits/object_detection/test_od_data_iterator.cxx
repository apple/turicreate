/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_od_data_iterator

#include <toolkits/object_detection/od_data_iterator.hpp>

#include <boost/test/unit_test.hpp>
#include <model_server/lib/image_util.hpp>
#include <core/util/test_macros.hpp>

namespace turi {
namespace object_detection {
namespace {

using neural_net::image_box;
using neural_net::labeled_image;

constexpr size_t IMAGE_HEIGHT = 64;
constexpr size_t IMAGE_WIDTH = 128;

// Returns an SFrame with columns "test_image" and "test_annotations".
data_iterator::parameters create_data(size_t num_rows) {

  data_iterator::parameters result;

  flex_list images(num_rows);
  flex_list annotations(num_rows);

  std::vector<unsigned char> buffer(IMAGE_HEIGHT * IMAGE_WIDTH * 3);
  for (size_t i = 0; i < num_rows; ++i) {

    // Each pixel has R, G, and B value equal to the row index (modulo 256).
    std::fill(buffer.begin(), buffer.end(),
              static_cast<unsigned char>(i % 256));
    images[i] = flex_image(reinterpret_cast<char*>(buffer.data()), IMAGE_HEIGHT,
                           IMAGE_WIDTH, 3, buffer.size(),
                           IMAGE_TYPE_CURRENT_VERSION,
                           static_cast<int>(Format::RAW_ARRAY));

    // Each image has one annotation, with the label "foo" and a bounding box
    // with height and width 16. As the row index increases, the box moves to
    // the right until eventually resetting to the left and moving down.
    annotations[i] = flex_list();
    annotations[i].push_back(flex_dict({
          {"label", "foo"},
          {"coordinates", flex_dict({ {"x", 8 + i % 112},
                                      {"y", 8 + i / 112},
                                      {"width", 16},
                                      {"height", 16}      })},
        }));
  }

  result.annotations_column_name = "test_annotations";
  result.image_column_name = "test_image";
  result.data =  gl_sframe({
      {"test_image", gl_sarray(images)},
      {"test_annotations", gl_sarray(annotations)},
  });
  result.shuffle = false;

  return result;
}

// TODO: Directly test data_iterator::convert_annotations_to_yolo, checking for
// corner cases.

BOOST_AUTO_TEST_CASE(test_simple_data_iterator) {

  static constexpr size_t NUM_ROWS = 4;
  static constexpr size_t BATCH_SIZE = 4;

  data_iterator::parameters params = create_data(NUM_ROWS);

  std::vector<std::string> class_labels = { "foo" };

  simple_data_iterator data_source(params);
  TS_ASSERT_EQUALS(data_source.class_labels(), class_labels);
  TS_ASSERT_EQUALS(data_source.num_instances(), 4);

  auto assert_batch = [](const std::vector<labeled_image>& batch,
                         size_t row_offset) {
    for (size_t i = 0; i < batch.size(); ++i) {

      const labeled_image& example = batch[i];
      size_t row = (row_offset + i) % NUM_ROWS;

      TS_ASSERT_EQUALS(example.image.m_height, IMAGE_HEIGHT);
      TS_ASSERT_EQUALS(example.image.m_width, IMAGE_WIDTH);
      TS_ASSERT_EQUALS(example.image.m_channels, 3);

      // The first byte of the first pixel should contain the row index.
      flex_image image = image_util::decode_image(example.image);
      TS_ASSERT_EQUALS(static_cast<size_t>(image.get_image_data()[0]),
                       row % 256);

      TS_ASSERT_EQUALS(example.annotations.size(), 1);
      TS_ASSERT_EQUALS(example.annotations[0].identifier, 0);
      TS_ASSERT_EQUALS(example.annotations[0].confidence, 1.f);
      TS_ASSERT_EQUALS(example.annotations[0].bounding_box,
                       image_box(static_cast<float>(row % 112) / IMAGE_WIDTH,
                                 static_cast<float>(row / 112) / IMAGE_HEIGHT,
                                 16.f / IMAGE_WIDTH,
                                 16.f / IMAGE_HEIGHT));
    }
  };

  size_t row_offset = 0;
  std::vector<labeled_image> batch = data_source.next_batch(BATCH_SIZE);
  TS_ASSERT_EQUALS(batch.size(), BATCH_SIZE);
  assert_batch(batch, row_offset);

  row_offset += BATCH_SIZE;
  batch = data_source.next_batch(BATCH_SIZE);
  TS_ASSERT_EQUALS(batch.size(), BATCH_SIZE);
  assert_batch(batch, 0);

}

BOOST_AUTO_TEST_CASE(test_simple_data_iterator_with_different_coordinate_systems) {

  auto create_data_opts = [](const std::string annotation_origin,
                         const std::string annotation_scale,
                         const std::string annotation_position,
                         float x, float y, float w, float h) {

    data_iterator::parameters result;

    flex_list images(1);
    flex_list annotations(1);
    std::vector<unsigned char> buffer(IMAGE_HEIGHT * IMAGE_WIDTH * 3);

    // Each pixel has R, G, and B value equal to the row index (modulo 256).
    std::fill(buffer.begin(), buffer.end(),
              static_cast<unsigned char>(0 % 256));
    images[0] = flex_image(reinterpret_cast<char*>(buffer.data()), IMAGE_HEIGHT,
                           IMAGE_WIDTH, 3, buffer.size(),
                           IMAGE_TYPE_CURRENT_VERSION,
                           static_cast<int>(Format::RAW_ARRAY));

    // Setting input for Image Origin
    if (annotation_origin == "top_left") {
        result.annotation_origin = data_iterator::annotation_origin_enum::TOP_LEFT;
    }
    if (annotation_origin == "bottom_left") {
        result.annotation_origin = data_iterator::annotation_origin_enum::BOTTOM_LEFT;
    }

    // Setting input for Annotation Scale
    if (annotation_scale == "pixel") {
        result.annotation_scale = data_iterator::annotation_scale_enum::PIXEL;
    }
    if (annotation_scale == "normalized") {
        result.annotation_scale = data_iterator::annotation_scale_enum::NORMALIZED;
    }

    // Setting input for Annotation Position
    if (annotation_position == "center") {
        result.annotation_position = data_iterator::annotation_position_enum::CENTER;
    }
    if (annotation_position == "top_left") {
        result.annotation_position = data_iterator::annotation_position_enum::TOP_LEFT;
    }
    if (annotation_position == "bottom_left") {
        result.annotation_position = data_iterator::annotation_position_enum::BOTTOM_LEFT;
    }

    // Each image has one annotation, with the label "foo" and a bounding box
    // with height and width 16. As the row index increases, the box moves to
    // the right until eventually resetting to the left and moving down.
    annotations[0] = flex_list();
    annotations[0].push_back(flex_dict({
          {"label", "foo"},
          {"coordinates", flex_dict({ {"x", x},
                                      {"y", y},
                                      {"width", w},
                                      {"height", h}      })},
        }));

    result.annotations_column_name = "test_annotations";
    result.image_column_name = "test_image";
    result.data =  gl_sframe({
        {"test_image", gl_sarray(images)},
        {"test_annotations", gl_sarray(annotations)},
    });

    result.shuffle = false;

    simple_data_iterator data_source(result);
    std::vector<labeled_image> batch = data_source.next_batch(1);
    const labeled_image& example = batch[0];

    return example;

  };

  // Case 1 | Input: ('top_left', 'pixel', 'center') → Output: ('top_left', 'normalized', 'top_left')
  const labeled_image& case1_example = create_data_opts("top_left", "pixel", "center", 28, 18, 16, 16);
  TS_ASSERT_EQUALS(case1_example.annotations[0].bounding_box,
                      image_box(static_cast<float>(20) / IMAGE_WIDTH,
                                 static_cast<float>(10) / IMAGE_HEIGHT,
                                 16.f / IMAGE_WIDTH,
                                 16.f / IMAGE_HEIGHT
                                 ));

  // Case 2 | Input: ('bottom_left', 'pixel', 'bottom_left') → Output: ('top_left', 'normalized', 'top_left')
  const labeled_image& case2_example = create_data_opts("bottom_left", "pixel", "bottom_left", 20, 38, 16, 16);
  TS_ASSERT_EQUALS(case2_example.annotations[0].bounding_box,
                      image_box(static_cast<float>(20) / IMAGE_WIDTH,
                                 static_cast<float>(10) / IMAGE_HEIGHT,
                                 16.f / IMAGE_WIDTH,
                                 16.f / IMAGE_HEIGHT
                                 ));

  // Case 3 | Input: ('bottom_left', 'normalized', 'bottom_left') → Output: ('top_left', 'normalized', 'top_left')
  const labeled_image& case3_example = create_data_opts("bottom_left", "normalized", "bottom_left", 20./IMAGE_WIDTH, 38./IMAGE_HEIGHT, 16./IMAGE_WIDTH, 16./IMAGE_HEIGHT);
  TS_ASSERT_EQUALS(case3_example.annotations[0].bounding_box,
                      image_box(static_cast<float>(20) / IMAGE_WIDTH,
                                 static_cast<float>(10) / IMAGE_HEIGHT,
                                 16.f / IMAGE_WIDTH,
                                 16.f / IMAGE_HEIGHT
                                 ));

  // Case 4 | Input: ('top_left', 'pixel', 'top_left') → Output: ('top_left', 'normalized', 'top_left')
  const labeled_image& case4_example = create_data_opts("top_left", "pixel", "top_left", 20, 10, 16, 16);
  TS_ASSERT_EQUALS(case4_example.annotations[0].bounding_box,
                      image_box(static_cast<float>(20) / IMAGE_WIDTH,
                                 static_cast<float>(10) / IMAGE_HEIGHT,
                                 16.f / IMAGE_WIDTH,
                                 16.f / IMAGE_HEIGHT
                                 ));

}

BOOST_AUTO_TEST_CASE(test_simple_data_iterator_with_expected_classes) {

  static constexpr size_t NUM_ROWS = 1;
  static constexpr size_t BATCH_SIZE = 1;

  data_iterator::parameters params = create_data(NUM_ROWS);

  std::vector<std::string> class_labels = { "bar", "foo" };
  params.class_labels = class_labels;

  simple_data_iterator data_source(params);
  TS_ASSERT_EQUALS(data_source.class_labels(), class_labels);
  TS_ASSERT_EQUALS(data_source.num_instances(), 1);

  std::vector<labeled_image> batch = data_source.next_batch(BATCH_SIZE);
  TS_ASSERT_EQUALS(batch.size(), BATCH_SIZE);

  // Even though the data only contained one label, "foo", it should receive
  // identifier 1 because we specified the class labels upfront.
  TS_ASSERT_EQUALS(batch[0].annotations.size(), 1);
  TS_ASSERT_EQUALS(batch[0].annotations[0].identifier, 1);
}

BOOST_AUTO_TEST_CASE(test_simple_data_iterator_with_unexpected_classes) {

  static constexpr size_t NUM_ROWS = 1;

  data_iterator::parameters params = create_data(NUM_ROWS);
  params.class_labels = { "bar" };

  // The data contains the label "foo", which is not among the expected class
  // labels.
  TS_ASSERT_THROWS_ANYTHING(simple_data_iterator unused_var(params));
}

}  // namespace
}  // namespace object_detection
}  // namespace turi
