/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/drawing_classifier/drawing_classifier.hpp>

#include <iostream>

#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/evaluation/metrics.hpp>


namespace turi {
namespace drawing_classifier {

namespace {

using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::model_backend;
using neural_net::model_spec;
using neural_net::shared_float_array;

}  // namespace

std::unique_ptr<model_spec> drawing_classifier::init_model() const {
  std::unique_ptr<model_spec> result(new model_spec);
  return result;
}

void drawing_classifier::train(gl_sframe data,
    std::string target_column_name,
    std::string feature_column_name,
    variant_type validation_data,
    std::map<std::string, flexible_type> opts) {
  nn_spec_ = init_model();
  /* TODO: Add code to train! */
}

gl_sarray drawing_classifier::predict(gl_sframe data, std::string output_type) {
  /* TODO: Add code to predict! */
  return gl_sarray();
}

gl_sframe drawing_classifier::predict_topk(gl_sframe data, 
  std::string output_type, size_t k) {
  /* TODO: Add code to predict_topk! */
  return gl_sframe();
}

variant_map_type drawing_classifier::evaluate(gl_sframe data, std::string metric) {
  // Perform prediction.
  gl_sarray predictions = predict(data, "probability_vector");

  /* TODO: This is just for the skeleton. Rewrite. */
  return evaluation::compute_classifier_metrics(
      data, "label", metric, predictions,
      {{"classes", 2}});
}

std::shared_ptr<coreml::MLModelWrapper> drawing_classifier::export_to_coreml(
    std::string filename) {
  /* Add code for export_to_coreml */
  return nullptr;
}

}  // namespace drawing_classifier
}  // namespace turi
