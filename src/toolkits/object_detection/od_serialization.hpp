/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_OBJECT_DETECTION_OD_SERIALIZATION_H_
#define TURI_OBJECT_DETECTION_OD_SERIALIZATION_H_

#include <ml/neural_net/model_spec.hpp>
#include <model_server/lib/extensions/ml_model.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>

namespace turi {
namespace object_detection {

void set_array_feature(CoreML::Specification::FeatureDescription* feature_desc, std::string name,
                       std::string short_description, const std::vector<size_t>& shape,
                       float value);

void _save_impl(oarchive& oarc,
                const std::map<std::string, variant_type>& state,
                const neural_net::float_array_map& weights);

void _load_version(iarchive& iarc, size_t version,
                   std::map<std::string, variant_type>* state,
                   neural_net::float_array_map* weights);

void init_darknet_yolo(neural_net::model_spec& nn_spec,
                       const size_t num_classes, size_t num_anchor_boxes,
                       const std::string& input_name);

neural_net::pipeline_spec export_darknet_yolo(
    const neural_net::float_array_map& weights, const std::string& input_name,
    const std::string& coordinates_name, const std::string& confidence_name,
    const std::vector<std::pair<float, float>>& anchor_boxes, size_t num_classes,
    bool use_nms_layer, size_t output_grid_height, size_t output_grid_width, float iou_threshold,
    float confidence_threshold, size_t spatial_reduction);

}  // namespace object_detection
}  // namespace turi

#endif  // TURI_OBJECT_DETECTION_OD_SERIALIZATION_H_
