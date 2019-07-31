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

namespace turi {
namespace object_detection {

void _save_impl(oarchive& oarc, const neural_net::model_spec& nn_spec,
                const std::map<std::string, variant_type>& state);

void _load_version(iarchive& iarc, size_t version,
                   neural_net::model_spec& nn_spec,
                   std::map<std::string, variant_type>& state,
                   const std::vector<std::pair<float, float>>& anchor_boxes);

void init_darknet_yolo(
    neural_net::model_spec& nn_spec, const size_t num_classes,
    const std::vector<std::pair<float, float>>& anchor_boxes);

}  // namespace object_detection
}  // namespace turi

#endif  // TURI_OBJECT_DETECTION_OD_SERIALIZATION_H_
