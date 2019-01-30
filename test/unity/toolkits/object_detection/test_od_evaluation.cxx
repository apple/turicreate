/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_od_evaluation

#include <unity/toolkits/object_detection/od_evaluation.hpp>

#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

namespace turi {
namespace object_detection {
namespace {

using neural_net::image_annotation;
using neural_net::image_box;

BOOST_AUTO_TEST_CASE(test_nms_with_empty_preds) {

  std::vector<image_annotation> empty;
  std::vector<image_annotation> result =
      apply_non_maximum_suppression(empty, 1.f);
  TS_ASSERT(result.empty());
}

BOOST_AUTO_TEST_CASE(test_nms_with_single_pred) {

  // Define one prediction with low confidence and arbitrary label and bounds.
  std::vector<image_annotation> predictions(1);
  predictions[0].confidence = 0.f;
  predictions[0].bounding_box = image_box(0.25f, 0.25f, 0.5f, 0.5f);

  // Should keep the low-confidence prediction.
  std::vector<image_annotation> result =
      apply_non_maximum_suppression(predictions, 0.5f);
  TS_ASSERT_EQUALS(result.size(), 1);
  TS_ASSERT(result[0] == predictions[0]);
}

BOOST_AUTO_TEST_CASE(test_nms_with_overlap_within_class) {

  // Define two predictions with the same class and overlapping bounding boxes.
  std::vector<image_annotation> predictions(2);
  predictions[0].identifier = 3;
  predictions[0].confidence = 0.75f;
  predictions[0].bounding_box = image_box(0.25f, 0.25f, 0.5f, 0.5f);
  predictions[1].identifier = 3;
  predictions[1].confidence = 0.95f;
  predictions[1].bounding_box = image_box(0.20f, 0.20f, 0.5f, 0.5f);

  // Should keep only the higher-confidence prediction.
  std::vector<image_annotation> result =
      apply_non_maximum_suppression(predictions, 0.5f);
  TS_ASSERT_EQUALS(result.size(), 1);
  TS_ASSERT(result[0] == predictions[1]);
}

BOOST_AUTO_TEST_CASE(test_nms_with_no_overlap_within_class) {

  // Define two predictions with the same class and minimal overlap.
  std::vector<image_annotation> predictions(2);
  predictions[0].identifier = 3;
  predictions[0].confidence = 0.75f;
  predictions[0].bounding_box = image_box(0.25f, 0.25f, 0.3f, 0.3f);
  predictions[1].identifier = 3;
  predictions[1].confidence = 0.95f;
  predictions[1].bounding_box = image_box(0.45f, 0.25f, 0.3f, 0.3f);

  // Should keep both predictions. (The higher-confidence prediction should now
  // come first.)
  std::vector<image_annotation> result =
      apply_non_maximum_suppression(predictions, 0.5f);
  TS_ASSERT_EQUALS(result.size(), 2);
  TS_ASSERT(result[0] == predictions[1]);
  TS_ASSERT(result[1] == predictions[0]);
}

BOOST_AUTO_TEST_CASE(test_nms_with_overlap_across_classes) {

  // Define two predictions with different classes and overlapping boxes.
  std::vector<image_annotation> predictions(2);
  predictions[0].identifier = 2;
  predictions[0].confidence = 0.75f;
  predictions[0].bounding_box = image_box(0.25f, 0.25f, 0.5f, 0.5f);
  predictions[1].identifier = 3;
  predictions[1].confidence = 0.95f;
  predictions[1].bounding_box = image_box(0.20f, 0.20f, 0.5f, 0.5f);

  // Should keep both predictions.
  std::vector<image_annotation> result =
      apply_non_maximum_suppression(predictions, 0.5f);
  TS_ASSERT_EQUALS(result.size(), 2);
  TS_ASSERT(result[0] == predictions[0]);
  TS_ASSERT(result[1] == predictions[1]);
}

}  // namespace
}  // namespace object_detection
}  // namespace turi
