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

constexpr char AP[] = "average_precision";
constexpr char AP50[] = "average_precision_50";

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

BOOST_AUTO_TEST_CASE(test_average_precision_iou_threshold) {

  // Ground truth label on entire unit square, for convenience.
  std::vector<image_annotation> gt_labels(1);
  gt_labels[0].identifier = 0;
  gt_labels[0].confidence = 1.f;
  gt_labels[0].bounding_box = image_box(0.f, 0.f, 1.f, 1.f);

  // Prediction with 62.5% overlap.
  std::vector<image_annotation> preds(1);
  preds[0].identifier = 0;
  preds[0].confidence = 0.5f;
  preds[0].bounding_box = image_box(0.f, 0.f, 0.625f, 1.f);

  // Compute metrics at IOU thresholds 0.5 and 0.75.
  average_precision_calculator calc(/* class_labels */ {"class0", "class1"},
                                    /* iou_thresholds */ { 0.5f, 0.75f });
  calc.add_row(preds, gt_labels);

  auto res = calc.evaluate();
  flex_dict ap50 = variant_get_value<flex_dict>(res[AP50]);
  flex_dict ap = variant_get_value<flex_dict>(res[AP]);
  TS_ASSERT_EQUALS(ap50.size(), 2);  // num_classes == 2
  TS_ASSERT_EQUALS(ap.size(), 2);  // num_classes == 2

  // For class 0, we should have AP 1.0 at IOU 50 and 0.0 at IOU 75
  TS_ASSERT_EQUALS(ap50[0].first, "class0");
  TS_ASSERT_EQUALS(ap50[0].second, 1.f);
  TS_ASSERT_EQUALS(ap[0].first, "class0");
  TS_ASSERT_EQUALS(ap[0].second, 0.5f);

  // For class 1, we had no ground truth labels, so AP is always 0.
  TS_ASSERT_EQUALS(ap50[1].first, "class1");
  TS_ASSERT_EQUALS(ap50[1].second, 0.f);
  TS_ASSERT_EQUALS(ap[1].first, "class1");
  TS_ASSERT_EQUALS(ap[1].second, 0.f);
}

BOOST_AUTO_TEST_CASE(test_average_precision_class_label_mismatch) {

  // Ground truth label on entire unit square, for convenience.
  std::vector<image_annotation> gt_labels(1);
  gt_labels[0].identifier = 0;
  gt_labels[0].confidence = 1.f;
  gt_labels[0].bounding_box = image_box(0.f, 0.f, 1.f, 1.f);

  // One prediction, overlapping the ground truth box but with the wrong label.
  std::vector<image_annotation> preds(1);
  preds[0].identifier = 1;
  preds[0].confidence = 0.9f;
  preds[0].bounding_box = image_box(0.f, 0.f, 0.625f, 1.f);

  // Compute metrics at IOU thresholds 0.5.
  average_precision_calculator calc(/* class_labels */ {"class0", "class1"},
                                    /* iou_thresholds */ { 0.5f });
  calc.add_row(preds, gt_labels);

  auto res = calc.evaluate();
  flex_dict ap50 = variant_get_value<flex_dict>(res[AP50]);
  flex_dict ap = variant_get_value<flex_dict>(res[AP]);
  TS_ASSERT_EQUALS(ap50.size(), 2);  // num_classes == 2
  TS_ASSERT_EQUALS(ap.size(), 2);  // num_classes == 2

  // AP should be 0 for both classes, since the one prediction and the one
  // ground truth label had different class labels.
  TS_ASSERT_EQUALS(ap50[0].second, 0.f);
  TS_ASSERT_EQUALS(ap[0].second, 0.f);
  TS_ASSERT_EQUALS(ap50[1].second, 0.f);
  TS_ASSERT_EQUALS(ap[1].second, 0.f);
}

BOOST_AUTO_TEST_CASE(test_average_precision_image_row_mismatch) {

  // Ground truth label on entire unit square, for convenience.
  std::vector<image_annotation> gt_labels(1);
  gt_labels[0].identifier = 0;
  gt_labels[0].confidence = 1.f;
  gt_labels[0].bounding_box = image_box(0.f, 0.f, 1.f, 1.f);

  // One prediction, overlapping the ground truth box but with the wrong label.
  std::vector<image_annotation> preds(1);
  preds[0].identifier = 0;
  preds[0].confidence = 0.9f;
  preds[0].bounding_box = image_box(0.f, 0.f, 1.f, 1.f);

  // Compute metrics at IOU thresholds 0.5.
  average_precision_calculator calc(/* class_labels */ {"class0"},
                                    /* iou_thresholds */ { 0.5f });
  calc.add_row(preds, {});
  calc.add_row({}, gt_labels);

  auto res = calc.evaluate();
  flex_dict ap50 = variant_get_value<flex_dict>(res[AP50]);
  flex_dict ap = variant_get_value<flex_dict>(res[AP]);
  TS_ASSERT_EQUALS(ap50.size(), 1);  // num_classes == 1
  TS_ASSERT_EQUALS(ap.size(), 1);  // num_classes == 1

  // The AP should be 0, since although the prediction was for image/row 0, and
  // only image/row1 had a labeled annotation.
  TS_ASSERT_EQUALS(ap50[0].second, 0.f);
  TS_ASSERT_EQUALS(ap[0].second, 0.f);
}

BOOST_AUTO_TEST_CASE(test_average_precision_overlapping_match) {

  // Two ground truth labels, one on the top half of the unit square and one on
  // the bottom half.
  std::vector<image_annotation> gt_labels(2);
  gt_labels[0].identifier = 0;
  gt_labels[0].confidence = 1.f;
  gt_labels[0].bounding_box = image_box(0.f, 0.f, 1.f, 0.5f);
  gt_labels[1].identifier = 0;
  gt_labels[1].confidence = 1.f;
  gt_labels[1].bounding_box = image_box(0.f, 0.5f, 1.f, 0.5f);

  // Three predictions, all overlapping one of the two ground-truth labels.
  std::vector<image_annotation> preds(3);
  preds[0].identifier = 0;
  preds[0].confidence = 0.9f;
  preds[0].bounding_box = image_box(0.f, 0.f, 1.f, 0.5f);
  preds[1].identifier = 0;
  preds[1].confidence = 0.75f;
  preds[1].bounding_box = image_box(0.f, 0.f, 1.f, 0.5f);
  preds[2].identifier = 0;
  preds[2].confidence = 0.5f;
  preds[2].bounding_box = image_box(0.f, 0.5f, 1.f, 0.5f);

  // Compute metrics at IOU thresholds 0.5.
  average_precision_calculator calc(/* class_labels */ {"class0"},
                                    /* iou_thresholds */ { 0.5f });
  calc.add_row(preds, gt_labels);

  auto res = calc.evaluate();
  flex_dict ap50 = variant_get_value<flex_dict>(res[AP50]);
  TS_ASSERT_EQUALS(ap50.size(), 1);  // num_classes == 1

  // For class 0, the AP should be an average of precision 1.0 for the first
  // matched label, and precision 0.666 for the second matched label, since only
  // one of the matches for first ground truth label can count.
  TS_ASSERT_DELTA(ap50[0].second, 5.f/6.f, 0.0001f);
}

BOOST_AUTO_TEST_CASE(test_average_precision_aggregate_across_rows) {

  // Compute metrics at IOU thresholds 0.5.
  average_precision_calculator calc(/* class_labels */ {"class0"},
                                    /* iou_thresholds */ { 0.5f });

  // Ground truth label on entire unit square, for convenience.
  std::vector<image_annotation> gt_labels(1);
  gt_labels[0].identifier = 0;
  gt_labels[0].confidence = 1.f;
  gt_labels[0].bounding_box = image_box(0.f, 0.f, 1.f, 1.f);

  // Two predictions, only the first of which will have IOU > 0.5
  std::vector<image_annotation> preds(2);
  preds[0].identifier = 0;
  preds[0].bounding_box = image_box(0.f, 0.f, 1.f, 0.75f);
  preds[1].identifier = 0;
  preds[1].bounding_box = image_box(0.f, 0.f, 1.f, 0.25f);

  // For the first row, use both predictions, giving the good prediction
  // higher confidence.
  preds[0].confidence = 0.9f;
  preds[1].confidence = 0.7f;
  calc.add_row(preds, gt_labels);

  // For the second row, only use the first prediction. Give it a low confidence
  preds.resize(1);
  preds[0].confidence = 0.5f;
  calc.add_row(preds, gt_labels);

  // Compute AP.
  auto res = calc.evaluate();
  flex_dict ap50 = variant_get_value<flex_dict>(res[AP50]);
  TS_ASSERT_EQUALS(ap50.size(), 1);  // num_classes == 1

  // The AP should be an average of precision 1.0 for the first matched label,
  // and precision 0.666 for the second matched label (in the second image/row),
  // since the bad prediction for the first image/row ranks higher.
  TS_ASSERT_DELTA(ap50[0].second, 5.f/6.f, 0.0001f);
}

BOOST_AUTO_TEST_CASE(test_average_precision_monotonic_precision) {

  // Compute metrics at IOU thresholds 0.5.
  average_precision_calculator calc(/* class_labels */ {"class0"},
                                    /* iou_thresholds */ { 0.5f });

  // Ground truth label on entire unit square, for convenience.
  std::vector<image_annotation> gt_labels(1);
  gt_labels[0].identifier = 0;
  gt_labels[0].confidence = 1.f;
  gt_labels[0].bounding_box = image_box(0.f, 0.f, 1.f, 1.f);

  // Two predictions, only the first of which will have IOU > 0.5
  std::vector<image_annotation> preds(2);
  preds[0].identifier = 0;
  preds[0].bounding_box = image_box(0.f, 0.f, 1.f, 0.75f);
  preds[1].identifier = 0;
  preds[1].bounding_box = image_box(0.f, 0.f, 1.f, 0.25f);

  // For the first row, use both predictions, but give the bad prediction the
  // highest confidence
  preds[0].confidence = 0.7f;
  preds[1].confidence = 0.9f;
  calc.add_row(preds, gt_labels);

  // For the second row, only use the first prediction. Give it a low confidence
  preds.resize(1);
  preds[0].confidence = 0.5f;
  calc.add_row(preds, gt_labels);

  // Compute AP.
  auto res = calc.evaluate();
  flex_dict ap50 = variant_get_value<flex_dict>(res[AP50]);
  TS_ASSERT_EQUALS(ap50.size(), 1);  // num_classes == 1

  // The AP should be 0.666, even though the precision upon matching the first
  // ground truth label is 0.5 (and the precision upon matching the second is
  // 0.666), since the user is assumed to prefer the point on the raw curve that
  // dominates in both precision and recall.
  TS_ASSERT_DELTA(ap50[0].second, 2.f/3.f, 0.0001f);
}

}  // namespace
}  // namespace object_detection
}  // namespace turi
