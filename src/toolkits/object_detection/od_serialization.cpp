/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/object_detection/od_serialization.hpp>

#include <stdlib.h>
#include <cstdio>

#include <toolkits/object_detection/od_yolo.hpp>

#include <toolkits/coreml_export/mlmodel_include.hpp>

namespace turi {
namespace object_detection {

namespace {

using CoreML::Specification::ArrayFeatureType;
using CoreML::Specification::FeatureDescription;
using CoreML::Specification::ImageFeatureType;
using CoreML::Specification::Model;
using CoreML::Specification::ModelDescription;
using CoreML::Specification::NeuralNetwork;
using CoreML::Specification::Pipeline;

using turi::neural_net::float_array_map;
using turi::neural_net::model_spec;
using turi::neural_net::pipeline_spec;
using turi::neural_net::zero_weight_initializer;

using padding_type = model_spec::padding_type;

constexpr char CONFIDENCE_STR[] =
    "Boxes × Class confidence (see user-defined metadata \"classes\")";
constexpr char COORDINATES_STR[] =
    "Boxes × [x, y, width, height] (relative to image size)";

}  // namespace

void _save_impl(oarchive& oarc,
                const std::map<std::string, variant_type>& state,
                const float_array_map& weights) {
  // Save model attributes.
  variant_deep_save(state, oarc);

  // Save neural net weights.
  oarc << weights;
}

void _load_version(iarchive& iarc, size_t version,
                   std::map<std::string, variant_type>* state,
                   float_array_map* weights) {
  // Load model attributes.
  variant_deep_load(*state, iarc);

  // Load neural net weights.
  iarc >> *weights;
}

void init_darknet_yolo(model_spec& nn_spec, size_t num_classes,
                       size_t num_anchor_boxes, const std::string& input_name) {
  int num_features = 3;

  // Scale pixel values 0..255 to [0,1]
  nn_spec.add_scale("_divscalar0", input_name, {1},
                    neural_net::scalar_weight_initializer(1 / 255.f));

  // Initialize Layer 0 to Layer 7
  const std::map<int, int> layer_num_to_channels = {
      {0, 16},  {1, 32},  {2, 64},   {3, 128},
      {4, 256}, {5, 512}, {6, 1024}, {7, 1024}};
  for (const auto& x : layer_num_to_channels) {
    std::string input_name;
    if (x.first == 0) {
      input_name = "_divscalar0";
    } else if (x.first == 7) {
      input_name = "leakyrelu6_fwd";
    } else {
      input_name = "pool" + std::to_string(x.first - 1) + "_fwd";
    }
    nn_spec.add_convolution(
        /* name */ "conv" + std::to_string(x.first) + "_fwd",
        /* input */ input_name,
        /* num_output_channels */ x.second,
        /* num_kernel_channels */ num_features,
        /* kernel_height */ 3,
        /* kernel_width */ 3,
        /* stride_height */ 1,
        /* stride_width */ 1,
        /* padding */ padding_type::SAME,
        /* weight_init_fn */ zero_weight_initializer());

    // Append batchnorm.
    nn_spec.add_batchnorm(
        /* name */ "batchnorm" + std::to_string(x.first) + "_fwd",
        /* input */ "conv" + std::to_string(x.first) + "_fwd",
        /* num_channels */ x.second,
        /* epsilon */ 0.00001f);

    // Append leakyrelu.
    nn_spec.add_leakyrelu(
        /* name */ "leakyrelu" + std::to_string(x.first) + "_fwd",
        /* input */ "batchnorm" + std::to_string(x.first) + "_fwd",
        /* alpha */ 0.1f);

    // Append Pooling for 0~5 layer
    if (x.first <= 5) {
      size_t stride = 2;
      padding_type pad_type = padding_type::VALID;
      bool use_poolexcludepadding = false;
      if (x.first == 5) {
        stride = 1;
        pad_type = padding_type::SAME;
        use_poolexcludepadding = true;
      }
      nn_spec.add_pooling("pool" + std::to_string(x.first) + "_fwd",
                          "leakyrelu" + std::to_string(x.first) + "_fwd", 2, 2,
                          stride, stride, pad_type, use_poolexcludepadding);
    }
    num_features = x.second;
  }

  // Append conv8.
  const size_t num_predictions = 5 + num_classes;  // Per anchor box
  const size_t conv8_c_out = num_anchor_boxes * num_predictions;
  nn_spec.add_convolution(/* name */ "conv8_fwd",
                          /* input */ "leakyrelu7_fwd",
                          /* num_output_channels */ conv8_c_out,
                          /* num_kernel_channels */ 1024,
                          /* kernel_height */ 1,
                          /* kernel_width */ 1,
                          /* stride_height */ 1,
                          /* stride_width */ 1,
                          /* padding */ padding_type::SAME,
                          /* weight_init_fn */ zero_weight_initializer(),
                          /* bias_init_fn */ zero_weight_initializer());

  // Add Preprocessing with image scale = 1.0
  // In order to make the format exactly the same with
  // object_detrector::init_model()
  nn_spec.add_preprocessing(input_name, 1.0);
}

pipeline_spec export_darknet_yolo(
    const float_array_map& weights, const std::string& input_name,
    const std::string& coordinates_name, const std::string& confidence_name,
    const std::vector<std::pair<float, float>>& anchor_boxes,
    size_t num_classes, size_t output_grid_height, size_t output_grid_width,
    size_t spatial_reduction) {
  // Initialize the result with the learned layers from the model_backend.
  std::unique_ptr<model_spec> nn_spec(new model_spec);
  init_darknet_yolo(*nn_spec, num_classes, anchor_boxes.size(), input_name);
  nn_spec->update_params(weights);

  // Add the layers that convert to intelligible predictions.
  add_yolo(nn_spec.get(), coordinates_name, confidence_name, "conv8_fwd",
           anchor_boxes, num_classes, output_grid_height, output_grid_width);

  // Extract the underlying Core ML spec and move it into a new Pipeline.
  std::unique_ptr<NeuralNetwork> network =
      std::move(*nn_spec).move_coreml_spec();
  std::unique_ptr<Pipeline> pipeline(new Pipeline);
  Model* model = pipeline->add_models();
  model->mutable_neuralnetwork()->Swap(network.get());

  // Write the ModelDescription.
  ModelDescription* model_desc = model->mutable_description();

  // Write FeatureDescription for the image input.
  FeatureDescription* input_desc = model_desc->add_input();
  input_desc->set_name(input_name);
  input_desc->set_shortdescription("Input image");
  ImageFeatureType* image_feature =
      input_desc->mutable_type()->mutable_imagetype();
  image_feature->set_width(output_grid_width * spatial_reduction);
  image_feature->set_height(output_grid_height * spatial_reduction);
  image_feature->set_colorspace(ImageFeatureType::RGB);

  // Create a helper function for writing the shapes of the confidence and
  // coordinates outputs.
  size_t num_predictions =
      output_grid_width * output_grid_height * anchor_boxes.size();
  auto set_shape = [num_predictions](FeatureDescription* feature_desc,
                                     size_t num_features_per_prediction) {
    ArrayFeatureType* array_feature =
        feature_desc->mutable_type()->mutable_multiarraytype();
    array_feature->set_datatype(ArrayFeatureType::DOUBLE);
    array_feature->add_shape(num_predictions);
    array_feature->add_shape(num_features_per_prediction);
  };

  // Write FeatureDescription for the confidence output.
  FeatureDescription* confidence_desc = model_desc->add_output();
  confidence_desc->set_name(confidence_name);
  confidence_desc->set_shortdescription(CONFIDENCE_STR);
  set_shape(confidence_desc, num_classes);

  // Write FeatureDescription for the coordinates output.
  FeatureDescription* coordinates_desc = model_desc->add_output();
  coordinates_desc->set_name(coordinates_name);
  coordinates_desc->set_shortdescription(COORDINATES_STR);
  set_shape(coordinates_desc, 4);

  // Set CoreML spec version.
  model->set_specificationversion(1);

  return pipeline_spec(std::move(pipeline));
}

}  // namespace object_detection
}  // namespace turi
