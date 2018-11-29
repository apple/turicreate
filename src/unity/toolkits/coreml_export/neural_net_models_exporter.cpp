/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/coreml_export/neural_net_models_exporter.hpp>

#include <logger/assertions.hpp>
#include <unity/toolkits/coreml_export/mlmodel_include.hpp>

using CoreML::Specification::ArrayFeatureType;
using CoreML::Specification::FeatureDescription;
using CoreML::Specification::ImageFeatureType;
using CoreML::Specification::ModelDescription;
using CoreML::Specification::NeuralNetworkLayer;
using turi::coreml::MLModelWrapper;

namespace turi {

std::shared_ptr<MLModelWrapper> export_object_detector_model(
    const neural_net::model_spec& nn_spec, size_t image_width,
    size_t image_height, size_t num_classes, size_t num_predictions,
    flex_dict user_defined_metadata) {

  CoreML::Specification::Model model_pb;

  // Scale pixel values 0..255 to [0,1]
  NeuralNetworkLayer* first_layer =
      model_pb.mutable_neuralnetwork()->add_layers();
  first_layer->set_name("_divscalar0");
  first_layer->add_input("image");
  first_layer->add_output("_divscalar0");
  first_layer->mutable_scale()->add_shapescale(1);
  first_layer->mutable_scale()->mutable_scale()->add_floatvalue(1 / 255.f);

  // Copy the NeuralNetwork layers from nn_spec.
  // TODO: This copies ~60MB at present. Should object_detector be responsible
  // for _divscalar0, even though this isn't performed by the cnn_module?
  model_pb.mutable_neuralnetwork()->MergeFrom(nn_spec.get_coreml_spec());
  ASSERT_GT(model_pb.neuralnetwork().layers_size(), 1);

  // Wire up the input layer from the copied layers to _divscalar0.
  // TODO: This assumes that the first copied layer is the (only) one to take
  // the input from "image".
  NeuralNetworkLayer* second_layer = 
      model_pb.mutable_neuralnetwork()->mutable_layers(1);
  ASSERT_EQ(second_layer->input_size(), 1);
  ASSERT_EQ(second_layer->input(0), "image");
  second_layer->set_input(0, "_divscalar0");

  // Write the ModelDescription.
  ModelDescription* model_desc = model_pb.mutable_description();

  // Write FeatureDescription for the image input.
  FeatureDescription* input_desc = model_desc->add_input();
  input_desc->set_name("image");
  input_desc->set_shortdescription("Input image");
  ImageFeatureType* input_image =
      input_desc->mutable_type()->mutable_imagetype();
  input_image->set_width(image_width);
  input_image->set_height(image_height);
  input_image->set_colorspace(ImageFeatureType::RGB);

  // Write FeatureDescription for the confidence output.
  FeatureDescription* confidence_desc = model_desc->add_output();
  confidence_desc->set_name("confidence");
  confidence_desc->set_shortdescription(
      "Boxes × Class confidence (see user-defined metadata \"classes\")");
  ArrayFeatureType* confidence_array =
      confidence_desc->mutable_type()->mutable_multiarraytype();
  confidence_array->add_shape(num_predictions);
  confidence_array->add_shape(num_classes);
  confidence_array->set_datatype(ArrayFeatureType::DOUBLE);

  // Write FeatureDescription for the coordinates output.
  FeatureDescription* coordinates_desc = model_desc->add_output();
  coordinates_desc->set_name("coordinates");
  coordinates_desc->set_shortdescription(
      "Boxes × [x, y, width, height] (relative to image size)");
  ArrayFeatureType* coordinates_array =
      coordinates_desc->mutable_type()->mutable_multiarraytype();
  coordinates_array->add_shape(num_predictions);
  coordinates_array->add_shape(4);
  coordinates_array->set_datatype(ArrayFeatureType::DOUBLE);

  // Set CoreML spec version.
  model_pb.set_specificationversion(1);

  auto model_wrapper = std::make_shared<MLModelWrapper>(
      std::make_shared<CoreML::Model>(model_pb));

  // Add metadata.
  model_wrapper->add_metadata({
      { "user_defined", std::move(user_defined_metadata) }
  });

  return model_wrapper;
}

}  // namespace turi
