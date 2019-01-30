/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/object_detection/od_evaluation.hpp>

#include <algorithm>

#include <logger/assertions.hpp>

namespace turi {
namespace object_detection {

using neural_net::image_annotation;
using neural_net::image_box;

namespace {

float compute_iou(const image_annotation& a, const image_annotation& b) {

  image_box intersection_box = a.bounding_box;
  intersection_box.clip(b.bounding_box);

  float intersection_area = intersection_box.area();
  float union_area =
      a.bounding_box.area() + b.bounding_box.area() - intersection_area;

  return intersection_area / union_area;
}

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
        return compute_iou(*class_it, ann) > iou_threshold;
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

}  // object_detection
}  // turi
