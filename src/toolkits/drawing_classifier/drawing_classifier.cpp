/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */


#include <iostream>
#include <random>
#include <memory>
#include <sstream>

#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/evaluation/metrics.hpp>
#include <toolkits/drawing_classifier/drawing_classifier.hpp>


namespace turi {
namespace drawing_classifier {

namespace {

using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::model_backend;
using neural_net::model_spec;
using neural_net::shared_float_array;
using neural_net::weight_initializer;
using neural_net::zero_weight_initializer;

using padding_type = model_spec::padding_type;
// annoymous helper sections

}  // namespace

std::unique_ptr<model_spec> drawing_classifier::init_model() const {
  std::unique_ptr<model_spec> result(new model_spec);

  // state is updated through init_train
  flex_string target = read_state<flex_string>("target");
  size_t num_classes = read_state<flex_int>("num_classes");

  // feature columns names
  const flex_list &features_list = read_state<flex_list>("features");

  result->add_channel_concat(
      "features",
      std::vector<std::string>(features_list.begin(), features_list.end()));

  weight_initializer initializer = zero_weight_initializer();

  std::string prefix{"drawing"};
  std::string input_name{"features"};
  std::string output_name;

  {
    size_t channels_filter = 16;
    size_t channels_kernel = 1;
    std::stringstream ss;

    for (size_t ii = 0; ii < 3; ii++) {
      if (ii) {
        input_name = std::move(output_name);
      }

      ss.str("");
      ss << prefix << "_conv" << ii << "_fwd";
      output_name = ss.str();

      result->add_convolution(
          /* name                */ output_name,
          /* input               */ input_name,
          /* num_output_channels */ channels_filter,
          /* num_kernel_channels */ channels_kernel,
          /* kernel_height       */ 3,
          /* kernel_width        */ 3,
          /* stride_height       */ 1,
          /* stride_width        */ 1,
          /* padding             */ padding_type::SAME,
          /* weight_init_fn      */ initializer,
          /* bias_init_fn        */ zero_weight_initializer());

      channels_kernel = channels_filter;
      channels_filter *= 2;

      input_name = std::move(output_name);
      ss.str("");
      ss << prefix << "_relu" << ii << "_fwd";
      output_name = ss.str();

      result->add_relu(output_name, input_name);

      input_name = std::move(output_name);
      ss.str("");
      ss << prefix << "_pool" << ii << "_fwd";
      output_name = ss.str();
      result->add_pooling(
        /* name                 */ output_name,
        /* input                */ input_name,
        /* kernel_height        */ 2,
        /* kernel_width         */ 2,
        /* stride_height        */ 2,
        /* stride_width         */ 2,
        /* padding              */ padding_type::VALID,
        /* avg excluded padding */ false);

    }
  }

  input_name = std::move(output_name);
  output_name = prefix + "_flatten0_fwd";
  result->add_flatten(output_name, input_name);

  input_name = std::move(output_name);
  output_name = prefix + "_dense0_fwd";

  result->add_inner_product(
      /* name                */ output_name,
      /* input               */ input_name,
      /* num_output_channels */ 64 * 3 * 3,
      /* num_input_channels  */ 128,
      /* weight_init_fn      */ initializer);

  input_name = std::move(output_name);
  output_name = prefix + "_dense1_fwd";

  result->add_inner_product(
      /* name                */ "dense_1",
      /* input               */ "dense_0",
      /* num_output_channels */ 128,
      /* num_input_channels  */ num_classes,
      /* weight_init_fn      */ initializer);

  input_name = std::move(output_name);
  result->add_softmax(target + "Probability", input_name);

  return result;
}

void drawing_classifier::train(gl_sframe data, std::string target_column_name,
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

variant_map_type drawing_classifier::evaluate(gl_sframe data,
                                              std::string metric) {
  // Perform prediction.
  gl_sarray predictions = predict(data, "probability_vector");

  /* TODO: This is just for the skeleton. Rewrite. */
  return evaluation::compute_classifier_metrics(data, "label", metric,
                                                predictions, {{"classes", 2}});
}

std::shared_ptr<coreml::MLModelWrapper> drawing_classifier::export_to_coreml(
    std::string filename) {
  /* Add code for export_to_coreml */
  return nullptr;
}

}  // namespace drawing_classifier
}  // namespace turi
