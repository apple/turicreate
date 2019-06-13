/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/object_detection/od_evaluation.hpp>

#include <algorithm>
#include <numeric>

#include <logger/assertions.hpp>

namespace turi {
namespace object_detection {

using neural_net::image_annotation;
using neural_net::image_box;

namespace {

constexpr char AP[] = "average_precision";
constexpr char MAP[] = "mean_average_precision";
constexpr char AP50[] = "average_precision_50";
constexpr char MAP50[] = "mean_average_precision_50";

float compute_iou(const image_box& a, const image_box& b) {

  image_box intersection_box = a;
  intersection_box.clip(b);

  float intersection_area = intersection_box.area();
  float union_area = a.area() + b.area() - intersection_area;

  return intersection_area / union_area;
}

// For computing average precision averaged over IOU thresholds from 50% to 95%,
// at intervals of 5%, as popularized by COCO.
std::vector<float> iou_thresholds_for_evaluation() {

  std::vector<float> result;

  for (float x = 50.f; x < 100.f; x += 5.f) {
    result.push_back(x / 100.f);
  }

  return result;
}

// Helper class used to compute the average precision for a particular class.
class precision_recall_curve {
public:

  // Requires the number of actual positive instances
  precision_recall_curve(size_t num_ground_truth_labels)
    : ground_truth_labels_used_(num_ground_truth_labels)
  {}

  // Registers a prediction not matched to a ground truth label.
  void add_false_positive() {
    ++num_predictions_;
  }

  // Registers a prediction matched to a ground truth label. The prediction will
  // only count as a true positive if that label hasn't been matched to a
  // previous (higher confidence) prediction.
  void add_true_positive_if_available(size_t ground_truth_label_index) {

    ++num_predictions_;

    if (ground_truth_labels_used_[ground_truth_label_index]) return;

    ground_truth_labels_used_[ground_truth_label_index] = true;

    float num_true_positives = precisions_.size() + 1;
    precisions_.push_back(num_true_positives / num_predictions_);
  }

  // Computes the average (across ground truth labels) of the precision required
  // to get that label as a true positive.
  float compute_average_precision() const {

    if (ground_truth_labels_used_.empty()) return 0.f;

    float sum = 0.f;
    float max_precision = 0.f;
    for (auto it = precisions_.rbegin(); it != precisions_.rend(); ++it) {

      // For this ground truth label, use the best precision that includes it.
      // This is the max of all precisions from this one in the vector.
      max_precision = std::max(max_precision, *it);
      sum += max_precision;
    }

    return sum / ground_truth_labels_used_.size();
  }

  bool has_total_recall() const {
    return precisions_.size() == ground_truth_labels_used_.size();
  }

private:

  size_t num_predictions_ = 0;
  std::vector<bool> ground_truth_labels_used_;
  std::vector<float> precisions_;
};

}  // namespace

std::vector<image_annotation> apply_non_maximum_suppression(
    std::vector<image_annotation> predictions, float iou_threshold) {

  // We're going to perform this algorithm in place, since it doesn't seem too
  // much more complex and allows us to avoid any memory allocations whatsoever.
  // First, sort the predictions first by class and then in descending order of
  // confidence.
  auto comparator = [](const image_annotation& a, const image_annotation& b) {
    if (a.identifier < b.identifier) return true;
    if (a.identifier > b.identifier) return false;
    return a.confidence > b.confidence;
  };
  std::sort(predictions.begin(), predictions.end(), comparator);

  // Invariant: the range (predictions.begin(), result_end) contains the results
  // for all classes processed so far.
  using iterator = std::vector<image_annotation>::iterator;
  iterator result_end = predictions.begin();

  // Iterate through each class label, one at a time.
  iterator class_begin = predictions.begin();
  while (class_begin != predictions.end()) {

    // Find the range corresponding to this class label.
    auto different_class = [=](const image_annotation& ann) {
      return ann.identifier != class_begin->identifier;
    };
    iterator class_end = std::find_if(class_begin + 1, predictions.end(),
                                      different_class);
    const iterator next_class_begin = class_end;

    // Filter the predictions for this range to remove overlaps.
    iterator class_it = class_begin;
    while (class_it != class_end) {

      // Remove lower-confidence predictions overlapping with *class_it.
      auto overlaps = [=](const image_annotation& ann) {
        float iou = compute_iou(class_it->bounding_box, ann.bounding_box);
        return iou > iou_threshold;
      };
      class_end = std::remove_if(class_it + 1, class_end, overlaps);

      // All non-overlapping predictions have been moved (if needed) to the
      // (possibly smaller) range (class_it, class_end). Whatever's left in
      // (class_end, next_class_begin) is just garbage now. Move on to the next
      // kept prediction.
      ++class_it;
    }

    // Add the remaining predictions for this class to our results.
    if (result_end == class_begin) {

      // We've kept everything so far, so our final results are still a prefix
      // of the sorted original predictions, plus whatever we've done for this
      // class.
      result_end = class_end;

    } else {

      // Copy the results for this class to the end of our accumulated results.
      // This really just moves these values to an earlier location in
      // `predictions`.
      result_end = std::copy(class_begin, class_end, result_end);
    }

    // What's left in the range (results_end, next_class_begin) is garbage that
    // can be overwritten by the next iteration.
    class_begin = next_class_begin;
  }

  // Take out the garbage.
  predictions.erase(result_end, predictions.end());

  return predictions;
}

average_precision_calculator::average_precision_calculator(
    flex_list class_labels, std::vector<float> iou_thresholds)
  : class_labels_(std::move(class_labels)),
    data_(class_labels_.size()),
    iou_thresholds_(std::move(iou_thresholds))
{}

average_precision_calculator::average_precision_calculator(
    flex_list class_labels)
  : average_precision_calculator(std::move(class_labels),
                                 iou_thresholds_for_evaluation())
{}

void average_precision_calculator::add_row(
    const std::vector<image_annotation>& predictions,
    const std::vector<image_annotation>& ground_truth) {

  // Keep track of which class_data values we're touching.
  std::vector<bool> class_updated(data_.size(), false);

  // Register all the model predictions.
  for (const image_annotation& pred_annotation : predictions) {

    int identifier = pred_annotation.identifier;
    class_updated[identifier] = true;

    // The next row index for this class is equal to the current size of the
    // ground_truth_indices vector.
    class_data& data = data_[identifier];
    data.predictions.emplace_back(pred_annotation.confidence,
                                  pred_annotation.bounding_box,
                                  data.ground_truth_indices.size());
  }

  // Register all the ground truth labels.
  for (const image_annotation& ground_truth_annotation : ground_truth) {

    int identifier = ground_truth_annotation.identifier;
    class_updated[identifier] = true;

    class_data& data = data_[identifier];
    data.ground_truth_boxes.push_back(ground_truth_annotation.bounding_box);
  }

  // For all updated class_values, register the new row index.
  for (size_t i = 0; i < class_updated.size(); ++i) {

    if (class_updated[i]) {

      class_data& data = data_[i];
      data.ground_truth_indices.push_back(data.ground_truth_boxes.size());
    }
  }
}

variant_map_type average_precision_calculator::evaluate() {

  std::vector<std::map<float,float>> raw_average_precisions;
  raw_average_precisions.reserve(data_.size());

  for (size_t i = 0; i < data_.size(); ++i) {
    raw_average_precisions.push_back(evaluate_class(i));
  }

  // Format the raw results into a variant_map_type.

  std::vector<float> average_precision_50(class_labels_.size());
  std::vector<float> average_precision(class_labels_.size());
  for (size_t i = 0; i < class_labels_.size(); ++i) {

    // Extract the average precision for this class at IOU threshold 50%.
    average_precision_50[i] = raw_average_precisions[i].at(0.5f);

    // Compute the average precision for this class averaged over all the IOU
    // threshold.
    float sum = 0.f;
    for (const auto& threshold_and_ap : raw_average_precisions[i]) {
      sum += threshold_and_ap.second;
    }
    average_precision[i] = sum /= raw_average_precisions[i].size();
  }

  // Store the requested summary statistics.
  auto create_dict = [this](const std::vector<float>& v) {
    flex_dict class_dict;
    class_dict.reserve(class_labels_.size());
    for (size_t i = 0; i < class_labels_.size(); ++i) {
      class_dict.emplace_back(class_labels_[i], v[i]);
    }
    return class_dict;
  };
  auto compute_mean = [](const std::vector<float>& v) {
    return std::accumulate(v.begin(), v.end(), 0.f) / v.size();
  };
  variant_map_type result_map = {
    {AP, create_dict(average_precision)},
    {AP50, create_dict(average_precision_50)},
    {MAP50, compute_mean(average_precision_50)},
    {MAP, compute_mean(average_precision)},
  };
  return result_map;
}

std::map<float,float> average_precision_calculator::evaluate_class(
    size_t identifier) {

  class_data& data = data_[identifier];
  std::vector<precision_recall_curve> pr_curves(
      iou_thresholds_.size(),
      precision_recall_curve(data.ground_truth_boxes.size()));

  // Rank the predictions by confidence.
  auto by_confidence = [](const prediction& a, const prediction& b) {
    return a.confidence > b.confidence;
  };
  std::sort(data.predictions.begin(), data.predictions.end(), by_confidence);
  for (const prediction& pred : data.predictions) {

    // Find ground truth labels for the image associated with this predictions.
    size_t gt_idx = 0;
    size_t gt_count;
    if (pred.row_index == 0) {
      gt_count = data.ground_truth_indices[pred.row_index];
    } else {
      gt_idx = data.ground_truth_indices[pred.row_index - 1];
      gt_count = data.ground_truth_indices[pred.row_index] - gt_idx;
    }

    // Find the ground truth label with the largest overlap with the prediction.
    float best_iou = 0.f;
    size_t best_gt_idx = 0;
    for (size_t i = 0; i < gt_count; ++i) {
      image_box gt_box = data.ground_truth_boxes[gt_idx + i];
      float iou = compute_iou(pred.bounding_box, gt_box);
      if (best_iou < iou) {
        best_iou = iou;
        best_gt_idx = gt_idx + i;
      }
    }

    // For each IOU threshold, register this prediction as a true positive or a
    // false positive, possibly depending on whether the matching ground-truth
    // label has already counted for a true positive.
    for (size_t i = 0; i < iou_thresholds_.size(); ++i) {
      if (best_iou >= iou_thresholds_[i]) {
        pr_curves[i].add_true_positive_if_available(best_gt_idx);
      } else {
        pr_curves[i].add_false_positive();
      }
    }
  }

  // Compute the average precisions and pack the results into a map.
  std::map<float, float> result;
  for (size_t i = 0; i < iou_thresholds_.size(); ++i) {
    result.emplace(iou_thresholds_[i],
                   pr_curves[i].compute_average_precision());
  }
  return result;
}

}  // object_detection
}  // turi
