/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef UNITY_TOOLKITS_COREML_EXPORT_NEURAL_NETS_MODELS_EXPORTER_HPP_
#define UNITY_TOOLKITS_COREML_EXPORT_NEURAL_NETS_MODELS_EXPORTER_HPP_

#include <map>
#include <memory>
#include <string>

#include <core/data/flexible_type/flexible_type.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <toolkits/coreml_export/mlmodel_wrapper.hpp>

namespace turi {

/**
 * Wraps a trained object detector model_spec as a complete MLModel.
 *
 * \param nn_spec The NeuralNetwork to wrap, which must accept an input "image"
 *            with shape (3, image_height, image_width) and values in [0,1],
 *            output "confidence" with shape (num_predictions, num_classes),
 *            and "coordinates" with shape (num_predictions, 4).
 * \todo Should model_spec include Model, not NeuralNetwork, so that the client
 *       is responsible for populating the inputs and outputs?
 */
std::shared_ptr<coreml::MLModelWrapper> export_object_detector_model(
    neural_net::pipeline_spec pipeline, size_t num_classes,
    size_t num_predictions, flex_list class_labels,
    std::map<std::string, flexible_type> options);

/** Wraps a trained activity classifier model_spec as a complete MLModel. */
std::shared_ptr<coreml::MLModelWrapper> export_activity_classifier_model(
    const neural_net::model_spec& nn_spec, size_t prediction_window,
    const flex_list& features, size_t lstm_hidden_layer_size,
    const flex_list& class_labels, const flex_string& target);

/** Wraps a trained style transfer model_spec as a complete MLModel. */
std::shared_ptr<coreml::MLModelWrapper> export_style_transfer_model(
    const neural_net::model_spec& nn_spec, size_t image_width,
    size_t image_height, bool include_flexible_shape,
    std::string content_feature, std::string style_feature, size_t num_styles);

/** Wraps a trained drawing classifier model_spec as a complete MLModel. */
std::shared_ptr<coreml::MLModelWrapper> export_drawing_classifier_model(
    const neural_net::model_spec& nn_spec, const flex_list& features,
    const flex_list& class_labels, const flex_string& target);

}  // namespace turi

#endif  // UNITY_TOOLKITS_COREML_EXPORT_NEURAL_NETS_MODELS_EXPORTER_HPP_
