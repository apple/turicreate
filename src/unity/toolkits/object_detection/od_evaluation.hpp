/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_OBJECT_DETECTION_OD_EVALUATION_H_
#define TURI_OBJECT_DETECTION_OD_EVALUATION_H_

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

}  // object_detection
}  // turi

#endif  // TURI_OBJECT_DETECTION_OD_EVALUATION_H_
