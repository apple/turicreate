/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_OBJECT_DETECTION_OD_YOLO_H_
#define TURI_OBJECT_DETECTION_OD_YOLO_H_

#include <ml/neural_net/image_augmentation.hpp>
#include <ml/neural_net/model_spec.hpp>

namespace turi {
namespace object_detection {

/**
 * Writes a list of image_annotation values into an output float buffer.
 *
 * \param annotations The list of annotations (for one image) to write
 * \param output_height The height of the YOLO output grid
 * \param output_width The width of the YOLO output grid
 * \param num_anchors The number of YOLO anchors
 * \param num_classes The number of classes in the output one-hot encoding
 * \param out Address to a float buffer of size output_height * output_width *
 *            num_anchors * (5 + num_classes)
 * \todo Add a mutable_float_array or shared_float_buffer type for functions
 *       like this one to write into.
 * \todo This strictly speaking doesn't belong in this data iterator type but
 *       probably doesn't warrant its own file yet (and would be nice not to
 *       bury in object_detector.cpp).
 */
void convert_annotations_to_yolo(
    const std::vector<neural_net::image_annotation>& annotations,
    size_t output_height, size_t output_width, size_t num_anchors,
    size_t num_classes, float* out);

/**
 * Parses the raw YOLO output map into annotations.
 *
 * \param yolo_map A float array with shape (H, W, B*(5+C)), where B is the
 *            number of anchors, C is the number of classes, and H and W are the
 *            height and width of the output grid.
 * \param anchor_boxes The B anchor boxes used to train the YOLO model, as a
 *            vector of (width, height) pairs (in the output grid coordinates).
 * \param min_confidence The smallest confidence score to allow in the returned
 *            results.
 * \return Annotations in the coordinate space of the output grid.
 */
std::vector<neural_net::image_annotation> convert_yolo_to_annotations(
    const neural_net::float_array& yolo_map,
    const std::vector<std::pair<float, float>>& anchor_boxes,
    float min_confidence);

/**
 * Appends layers to an existing neural net spec, implementing the conversion
 * from a trained YOLO model to predicted bounding boxes and class labels.
 *
 * \param nn_spec Model spec for the trained YOLO model
 * \param coordinates_name The name to give to the CoreML layer which will
 *            output the predicted bounding boxes (B*H*W, 4, 1) for each of the
 *            B anchor boxes and each of the H*W output grid cells, in
 *            (x,y,width,height) order, normalized to the interval [0,1].
 * \param confidence_name The name to give to the CoreML layer which will output
 *            the predicted class label confidences (B*H*W, C, 1) for each of
 *            the B anchor boxes, each of the H*W output grid cells, and each of
 *            the C class labels.
 * \param input The name of the existing CoreML layer that outputs the raw
 *            (B*(5+C), H, W) predictions of the trained model: for each of B
 *            anchor boxes, the (x, y, width, height) bounding box, object
 *            confidence, and C class label confidences, for each of the H*W
 *            output grid cells.
 * \param anchor_boxes The B anchor boxes used to train the YOLO model, as a
 *            vector of (width, height) pairs (in the output grid coordinates).
 * \param num_classes The number of class labels C used to train the YOLO model.
 * \param output_grid_height The height H of the output grid used to train the
 *            YOLO model.
 * \param output_grid_width The width W of the output grid used to train the
 *            YOLO model.
 * \param prefix The prefix to apply to intermediate layers added in service of
 *            output layers named by `coordinates_name` and `confidence_name`.
 */
void add_yolo(neural_net::model_spec* nn_spec,
              const std::string& coordinates_name,
              const std::string& confidence_name, const std::string& input,
              const std::vector<std::pair<float, float>>& anchor_boxes,
              size_t num_classes, size_t output_grid_height,
              size_t output_grid_width,
              std::string prefix = "__tc__internal__");

}  // object_detection
}  // turi

#endif  // TURI_OBJECT_DETECTION_OD_YOLO_H_
