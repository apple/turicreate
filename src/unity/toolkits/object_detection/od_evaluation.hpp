/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_OBJECT_DETECTION_OD_EVALUATION_H_
#define TURI_OBJECT_DETECTION_OD_EVALUATION_H_

#include <vector>

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
   * \param num_classes The number of class labels. Each prediction and ground
   *            truth annotation must have a nonnegative identifier smaller than
   *            this value.
   * \param iou_thresholds The IOU (intersection over union) thresholds at which
   *            to compute the average precisions. This threshold determines
   *            whether a predicted bounding box and a ground truth bounding box
   *            are considered to match.
   */
  average_precision_calculator(size_t num_classes,
                               std::vector<float> iou_thresholds);

  /**
   * Registers the predictions and ground truth annotations for one image.
   */
  void add_row(const std::vector<neural_net::image_annotation>& predictions,
               const std::vector<neural_net::image_annotation>& ground_truth);

  /**
   * Computes the average precision for each combination of class and requested
   * IOU threshold.
   *
   * \return A vector, indexed by class identifier, of maps associating IOU
   *         threshold to average precision.
   *
   * Each value is the average precision for a specific class label with a
   * specific IOU threshold. This metric can be interpreted as the area under
   * the precision-recall curve.
   */
  std::vector<std::map<float,float>> evaluate();

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

  std::vector<class_data> data_;
  std::vector<float> iou_thresholds_;
};

}  // object_detection
}  // turi

#endif  // TURI_OBJECT_DETECTION_OD_EVALUATION_H_
