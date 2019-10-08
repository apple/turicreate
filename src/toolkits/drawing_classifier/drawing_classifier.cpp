/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/drawing_classifier/drawing_classifier.hpp>

// #include <algorithm>
// #include <functional>
// #include <numeric>
// #include <random>
#include <iostream>

#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
// #include <toolkits/coreml_export/neural_net_models_exporter.hpp>
// #include <toolkits/evaluation/metrics.hpp>
// #include <core/util/string_util.hpp>


namespace turi {
namespace drawing_classifier {

namespace {

using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::model_backend;
using neural_net::model_spec;
using neural_net::shared_float_array;

}  // namespace

std::unique_ptr<compute_context> drawing_classifier::create_compute_context()
    const {
  return compute_context::create_tf();
}

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
  // Instantiate the compute context.
  training_compute_context_ = create_compute_context();
  if (training_compute_context_ == nullptr) {
    log_and_throw("No neural network compute context provided");
  }
  // TODO: Do not hardcode values
  training_model_ = training_compute_context_->create_drawing_classifier(
    validation_data,
    /* TODO: nn_spec_->export_params_view().
     * Until the nn_spec in C++ isn't ready, do not pass in any weights. 
     */
    256,
    2,
    true
  );
}

gl_sarray drawing_classifier::predict(gl_sframe data, std::string output_type) {
  if (output_type.empty()) {
    output_type = "class";
  }
  if (output_type != "class" && output_type != "probability_vector") {
    log_and_throw(output_type
      + " is not a valid option for output_type. " 
      + "Expected one of: probability_vector, class");
  }
  return gl_sarray();
}

}  // namespace drawing_classifier
}  // namespace turi
