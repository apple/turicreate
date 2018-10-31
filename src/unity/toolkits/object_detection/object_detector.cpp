/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/object_detection/object_detector.hpp>

#include <logger/logger.hpp>
#include <unity/toolkits/neural_net/coreml_import.hpp>

using turi::neural_net::cnn_module;
using turi::neural_net::float_array_map;
using turi::neural_net::load_network_params;
using turi::neural_net::shared_float_array;

namespace turi {
namespace object_detection {

namespace {

constexpr size_t OBJECT_DETECTOR_VERSION = 1;

// TODO: Batch size should be a user-configurable parameter.
constexpr int BATCH_SIZE = 32;

// We assume RGB input.
constexpr int NUM_INPUT_CHANNELS = 3;

// TODO: This should be computed dynamically from the number of class labels and
// the number of anchor boxes. The current value assumes 15 anchor boxes and
// 11 outputs per anchor (4 bounding-box coordinates + 1 confidence + 6 labels).
constexpr int NUM_OUTPUT_CHANNELS = 165;

// Annotated and predicted bounding boxes are defined relative to a
// GRID_SIZE x GRID_SIZE grid laid over the image.
constexpr int GRID_SIZE = 13;

// The spatial reduction depends on the input size of the pre-trained model
// (relative to the grid size).
// TODO: When we support alternative base models, we will have to generalize.
constexpr int SPATIAL_REDUCTION = 32;

// These are the fixed values that the Python implementation currently passes
// into TCMPS.
// TODO: These should be exposed in a way that facilitates experimentation.
// TODO: A struct instead of a map would be nice, too.
float_array_map get_training_config() {
  float_array_map config;
  config["gradient_clipping"]        = shared_float_array::wrap(0.2f);
  config["learning_rate"]            = shared_float_array::wrap(0.000125f);
  config["mode"]                     = shared_float_array::wrap(0.f);
  config["od_include_loss"]          = shared_float_array::wrap(1.0f);
  config["od_include_network"]       = shared_float_array::wrap(1.0f);
  config["od_max_iou_for_no_object"] = shared_float_array::wrap(0.3f);
  config["od_min_iou_for_object"]    = shared_float_array::wrap(0.7f);
  config["od_rescore"]               = shared_float_array::wrap(1.0f);
  config["od_scale_class"]           = shared_float_array::wrap(16.0f);
  config["od_scale_no_object"]       = shared_float_array::wrap(40.0f);
  config["od_scale_object"]          = shared_float_array::wrap(800.0f);
  config["od_scale_wh"]              = shared_float_array::wrap(80.0f);
  config["od_scale_xy"]              = shared_float_array::wrap(80.0f);
  config["use_sgd"]                  = shared_float_array::wrap(1.0f);
  config["weight_decay"]             = shared_float_array::wrap(0.0005f);
  return config;
}

}  // namespace

object_detector::object_detector()
  : object_detector(&load_network_params, &cnn_module::create_object_detector)
{}

object_detector::object_detector(coreml_importer coreml_importer_fn,
                                 module_factory module_factory_fn)
  : coreml_importer_fn_(std::move(coreml_importer_fn)),
    module_factory_fn_(std::move(module_factory_fn))
{}

void object_detector::init_options(const std::map<std::string,
                                   flexible_type>& options) {}

size_t object_detector::get_version() const {
  return OBJECT_DETECTOR_VERSION;
}

void object_detector::save_impl(oarchive& oarc) const {}

void object_detector::load_version(iarchive& iarc, size_t version) {}

void object_detector::train(gl_sframe data,
                            std::string annotations_column_name,
                            std::string image_column_name,
                            std::map<std::string, flexible_type> options) {
  auto options_iter = options.find("model_params_path");
  if (options_iter == options.end()) {
    log_and_throw("Expected option \"model_params_path\" not found.");
  }
  const std::string model_params_path = options_iter->second;
  float_array_map model_params = coreml_importer_fn_(model_params_path);

  training_module_ = module_factory_fn_(
      BATCH_SIZE, NUM_INPUT_CHANNELS, /* h_in */ GRID_SIZE * SPATIAL_REDUCTION,
      /* w_in */ GRID_SIZE * SPATIAL_REDUCTION, NUM_OUTPUT_CHANNELS,
      /* h_out */ GRID_SIZE, /* w_out */ GRID_SIZE, get_training_config(),
      std::move(model_params));
}

}  // object_detection
}  // turi 

