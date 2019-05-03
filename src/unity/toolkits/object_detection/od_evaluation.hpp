/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_OBJECT_DETECTION_OD_EVALUATION_H_
#define TURI_OBJECT_DETECTION_OD_EVALUATION_H_

#include <vector>

#include <unity/lib/variant.hpp>
#include <unity/toolkits/neural_net/image_augmentation.hpp>

namespace turi {
namespace object_detection {

/**
 * Performs class-independent non-maximum suppression on the given predictions.
 *
 * \param predictions A collection of possibly overlapping predictions
 * \param iou_threshold The maximum allowed overlap (computed as the ratio
 *            between the intersection area and the union area) between any two
 *            predictions for the same class
 * \return A subset of the given predictions, removing overlapping results,
 *         greedily preferring those with the highest confidence.
 */
std::vector<neural_net::image_annotation> apply_non_maximum_suppression(
    std::vector<neural_net::image_annotation> predictions, float iou_threshold);

/**
 * Helper class for computing AP and mAP metrics.
 */
class average_precision_calculator {
public:

  /**
   * \param class_labels Each prediction and ground truth annotation must have a
                nonnegative identifier indexing into this list.
   * \param iou_thresholds The IOU (intersection over union) thresholds at which
   *            to compute the average precisions. This threshold determines
   *            whether a predicted bounding box and a ground truth bounding box
   *            are considered to match.
   */
  average_precision_calculator(flex_list class_labels,
                               std::vector<float> iou_thresholds);

  // Variant of above that uses the default list of iou_thresholds, ranging from
  // 0.5 to 0.95 with a step size of 0.05.
  explicit average_precision_calculator(flex_list class_labels);

  /**
   * Registers the predictions and ground truth annotations for one image.
   */
  void add_row(const std::vector<neural_net::image_annotation>& predictions,
               const std::vector<neural_net::image_annotation>& ground_truth);

  /**
   * Computes the average precision for each combination of class and requested
   * IOU threshold.
   *
   * \return A map of evaluation results keyed by metric.
   *
   * The average precision can be interpreted as the area under the
   * precision-recall curve.
   *
   * average_precision_50 is a dictionary mapping class label to the average
   * precision for that class label at 50% IOU.
   *
   * average_precision is a dictionary mapping class label to the average
   * precision for that class label, average across IOU thresholds from 50% to
   * 95%.
   *
   * mean_average_precision_50 is the mean across class labels of the
   * average_precision_50 values.
   *
   * mean_average_precision is the mean across class labels of the
   * average_precision values.
   */
  variant_map_type evaluate();

private:

  std::map<float, float> evaluate_class(size_t identifier);

  // Representation of one model prediction (for a given class).
  struct prediction {
    prediction(float confidence, neural_net::image_box bounding_box,
               size_t row_index)
      : confidence(confidence), bounding_box(std::move(bounding_box)),
        row_index(row_index)
    {}

    float confidence;
    neural_net::image_box bounding_box;
    size_t row_index;
  };

  // All the data relevant to computing average precision for a single class.
  struct class_data {

    // All the predictions with the class's label.
    std::vector<prediction> predictions;

    // All the ground truth bounding boxes for the class.
    std::vector<neural_net::image_box> ground_truth_boxes;

    // Given row index `i`, the elements of ground_truth_boxes associated with
    // that row begin with `ground_truth_boxes[ground_truth_indices[i - 1]]` and
    // end just before `ground_truth_boxes[ground_truth_indices[i]]`.
    std::vector<size_t> ground_truth_indices;
  };

  flex_list class_labels_;
  std::vector<class_data> data_;
  std::vector<float> iou_thresholds_;
};

}  // object_detection
}  // turi

#endif  // TURI_OBJECT_DETECTION_OD_EVALUATION_H_
