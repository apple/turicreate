/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/coreml_export/neural_net_models_exporter.hpp>

#include <logger/assertions.hpp>
#include <unity/toolkits/coreml_export/mlmodel_include.hpp>

#include <unity/toolkits/coreml_export/mldata_exporter.hpp>
#include <unity/toolkits/coreml_export/coreml_export_utils.hpp>

using CoreML::Specification::ArrayFeatureType;
using CoreML::Specification::DoubleFeatureType;
using CoreML::Specification::FeatureDescription;
using CoreML::Specification::ImageFeatureType;
using CoreML::Specification::ModelDescription;
using CoreML::Specification::NeuralNetworkLayer;
using turi::coreml::MLModelWrapper;

  
namespace turi {

std::shared_ptr<MLModelWrapper> export_object_detector_model(
    const neural_net::model_spec& nn_spec, size_t image_width,
    size_t image_height, size_t num_classes, size_t num_predictions,
    flex_dict user_defined_metadata, flex_list class_labels, std::map<std::string, flexible_type> options) {

  
  CoreML::Specification::Model model_nn;

  // Scale pixel values 0..255 to [0,1]
  NeuralNetworkLayer* first_layer =
      model_nn.mutable_neuralnetwork()->add_layers();
  first_layer->set_name("_divscalar0");
  first_layer->add_input("image");
  first_layer->add_output("_divscalar0");
  first_layer->mutable_scale()->add_shapescale(1);
  first_layer->mutable_scale()->mutable_scale()->add_floatvalue(1 / 255.f);

  // Copy the NeuralNetwork layers from nn_spec.
  // TODO: This copies ~60MB at present. Should object_detector be responsible
  // for _divscalar0, even though this isn't performed by the cnn_module?
  model_nn.mutable_neuralnetwork()->MergeFrom(nn_spec.get_coreml_spec());
  ASSERT_GT(model_nn.neuralnetwork().layers_size(), 1);

  // Wire up the input layer from the copied layers to _divscalar0.
  // TODO: This assumes that the first copied layer is the (only) one to take
  // the input from "image".
  NeuralNetworkLayer* second_layer = 
      model_nn.mutable_neuralnetwork()->mutable_layers(1);
  ASSERT_EQ(second_layer->input_size(), 1);
  ASSERT_EQ(second_layer->input(0), "image");
  second_layer->set_input(0, "_divscalar0");

  // Write the ModelDescription.
  ModelDescription* model_desc = model_nn.mutable_description();

  // Write FeatureDescription for the image input.
  FeatureDescription* input_desc = model_desc->add_input();
  input_desc->set_name("image");
  //input_desc->set_shortdescription("Input image");
  ImageFeatureType* input_image =
      input_desc->mutable_type()->mutable_imagetype();
  input_image->set_width(image_width);
  input_image->set_height(image_height);
  input_image->set_colorspace(ImageFeatureType::RGB);

  if (options["include_non_maximum_suppression"] != "True") {
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
    FeatureDescription* output_coordinates_pb = model_desc->add_output();
    output_coordinates_pb->set_name("coordinates");
    output_coordinates_pb->set_shortdescription(
        "Boxes × [x, y, width, height] (relative to image size)");
    ArrayFeatureType* output_coordinates_pb_feature =
        output_coordinates_pb->mutable_type()->mutable_multiarraytype();
    output_coordinates_pb_feature->add_shape(num_predictions);
    output_coordinates_pb_feature->add_shape(4);
    output_coordinates_pb_feature->set_datatype(ArrayFeatureType::DOUBLE);

    // Set CoreML spec version.
    model_nn.set_specificationversion(1);
    auto model_wrapper = std::make_shared<MLModelWrapper>(
      std::make_shared<CoreML::Model>(model_nn));

    // Add metadata.
    model_wrapper->add_metadata({
        { "user_defined", std::move(user_defined_metadata) }
    });

    return model_wrapper;
  }
  
  model_nn.set_specificationversion(3);

  // Write FeatureDescription for the raw confidence output.
  FeatureDescription* output_rconfidence_pb = model_desc->add_output();
  output_rconfidence_pb->set_name("raw_confidence");
  ArrayFeatureType* output_rconfidence_feature =
      output_rconfidence_pb->mutable_type()->mutable_multiarraytype();
  output_rconfidence_feature->add_shape(num_predictions);
  output_rconfidence_feature->add_shape(num_classes);
  output_rconfidence_feature->set_datatype(ArrayFeatureType::DOUBLE);

  auto *rconfidence_shape_pb1 = output_rconfidence_feature->mutable_shaperange()->add_sizeranges();
  rconfidence_shape_pb1->set_upperbound(-1);
  auto *rconfidence_shape_pb2 = output_rconfidence_feature->mutable_shaperange()->add_sizeranges();
  rconfidence_shape_pb2->set_lowerbound(6);
  rconfidence_shape_pb2->set_upperbound(6);

  // Write FeatureDescription for the coordinates output.
  FeatureDescription* output_rcoordinates_pb = model_desc->add_output();
  output_rcoordinates_pb->set_name("raw_coordinates");
  ArrayFeatureType* output_rcoordinates_pb_feature =
      output_rcoordinates_pb->mutable_type()->mutable_multiarraytype();
  output_rcoordinates_pb_feature->add_shape(num_predictions);
  output_rcoordinates_pb_feature->add_shape(4);
  output_rcoordinates_pb_feature->set_datatype(ArrayFeatureType::DOUBLE);

  auto *rcoordinates_shape_pb1 = output_rcoordinates_pb_feature->mutable_shaperange()
      ->add_sizeranges();
  rcoordinates_shape_pb1->set_upperbound(-1);
  auto *rcoordinates_shape_pb2 = output_rcoordinates_pb_feature->mutable_shaperange()
      ->add_sizeranges();
  rcoordinates_shape_pb2->set_lowerbound(4);
  rcoordinates_shape_pb2->set_upperbound(4);
  
  // Create Non Maximum Suppression model
  CoreML::Specification::Model model_nms;
  model_nms.set_specificationversion(3);

  ModelDescription* nms_desc = model_nms.mutable_description();

  // Write FeatureDescription for the Raw Confidence input.
  FeatureDescription* input_rconfidence_nms = nms_desc->add_input();
  input_rconfidence_nms->set_name("raw_confidence");
  ArrayFeatureType* input_rconfidence_nms_feature =
      input_rconfidence_nms->mutable_type()->mutable_multiarraytype();
  input_rconfidence_nms_feature->add_shape(num_predictions);
  input_rconfidence_nms_feature->add_shape(num_classes);
  input_rconfidence_nms_feature->set_datatype(ArrayFeatureType::DOUBLE);

  auto *rconfidence_shape_nms1 = input_rconfidence_nms_feature->mutable_shaperange()
      ->add_sizeranges();
  rconfidence_shape_nms1->set_upperbound(-1);
  auto *rconfidence_shape_nms2 = input_rconfidence_nms_feature->mutable_shaperange()
      ->add_sizeranges();
  rconfidence_shape_nms2->set_lowerbound(6);
  rconfidence_shape_nms2->set_upperbound(6);
  
  // Write FeatureDescription for the Raw Coordinates input.
  FeatureDescription* input_rcoordinates_nms = nms_desc->add_input();
  input_rcoordinates_nms->set_name("raw_coordinates"); 

  ArrayFeatureType* input_rcoordinates_nms_feature =
      input_rcoordinates_nms->mutable_type()->mutable_multiarraytype();
  input_rcoordinates_nms_feature->add_shape(num_predictions);
  input_rcoordinates_nms_feature->add_shape(4);
  input_rcoordinates_nms_feature->set_datatype(ArrayFeatureType::DOUBLE);

  auto *rcoordinates_shape_nms1 = input_rcoordinates_nms_feature->mutable_shaperange()->add_sizeranges();
  rcoordinates_shape_nms1->set_upperbound(-1);
  auto *rcoordinates_shape_nms2 = input_rcoordinates_nms_feature->mutable_shaperange()->add_sizeranges();
  rcoordinates_shape_nms2->set_lowerbound(4);
  rcoordinates_shape_nms2->set_upperbound(4);


  // Write FeatureDescription for the IOU Threshold input.
  FeatureDescription* input_iou_nms = nms_desc->add_input();
  input_iou_nms->set_name("iouThreshold");
  input_iou_nms->mutable_type()->mutable_doubletype();

  // Write FeatureDescription for the Confidence Threshold input.
  FeatureDescription* input_cthreshold_nms = nms_desc->add_input();
  input_cthreshold_nms->set_name("confidenceThreshold");
  input_cthreshold_nms->mutable_type()->mutable_doubletype();

  // Write FeatureDescription for the Confidence output.
  FeatureDescription* output_confidence_nms = nms_desc->add_output();
  output_confidence_nms->set_name("confidence");

  ArrayFeatureType* output_confidence_nms_feature =
      output_confidence_nms->mutable_type()->mutable_multiarraytype();
  output_confidence_nms_feature->set_datatype(ArrayFeatureType::DOUBLE);

  auto *confidence_shape_nms1 = output_confidence_nms_feature->mutable_shaperange()
      ->add_sizeranges();
  confidence_shape_nms1->set_upperbound(-1);
  auto *confidence_shape_nms2 = output_confidence_nms_feature->mutable_shaperange()
      ->add_sizeranges();
  confidence_shape_nms2->set_lowerbound(6);
  confidence_shape_nms2->set_upperbound(6);

  // Write FeatureDescription for the Coordinates input.
  FeatureDescription* output_coordinates_nms = nms_desc->add_output();
  output_coordinates_nms->set_name("coordinates");

  ArrayFeatureType* output_coordinates_nms_feature =
      output_coordinates_nms->mutable_type()->mutable_multiarraytype();
  output_coordinates_nms_feature->set_datatype(ArrayFeatureType::DOUBLE);

  auto *coordinates_shape_nms1 = output_coordinates_nms_feature->mutable_shaperange()
      ->add_sizeranges();
  coordinates_shape_nms1->set_upperbound(-1);
  auto *coordinates_shape_nms2 = output_coordinates_nms_feature->mutable_shaperange()
      ->add_sizeranges();
  coordinates_shape_nms2->set_lowerbound(4);
  coordinates_shape_nms2->set_upperbound(4);
  
  CoreML::Specification::NonMaximumSuppression* first_layer_nms =
    model_nms.mutable_nonmaximumsuppression();
  
  // Write Class Labels
  auto* string_class_labels = first_layer_nms->mutable_stringclasslabels(); 
  for (size_t i = 0; i < class_labels.size(); ++i) {
    string_class_labels->add_vector(class_labels[i]);
  }

  //Write Features for Non Maximum Suppression
  first_layer_nms->set_iouthreshold(options["iou_threshold"]);
  first_layer_nms->set_confidencethreshold(options["confidence_threshold"]);
  first_layer_nms->set_confidenceinputfeaturename("raw_confidence");
  first_layer_nms->set_coordinatesinputfeaturename("raw_coordinates");
  first_layer_nms->set_iouthresholdinputfeaturename("iouThreshold");
  first_layer_nms->set_confidencethresholdinputfeaturename("confidenceThreshold");
  first_layer_nms->set_confidenceoutputfeaturename("confidence");
  first_layer_nms->set_coordinatesoutputfeaturename("coordinates");


  // Set up Pipeline 
  CoreML::Specification::Model model_pipeline;
  model_pipeline.set_specificationversion(3); 
  ModelDescription* pipeline_desc = model_pipeline.mutable_description();

  // Write FeatureDescription for the image input.
  FeatureDescription* input_image_pipeline = pipeline_desc->add_input();
  input_image_pipeline->set_name("image");
  input_image_pipeline->set_shortdescription("Input image");
  ImageFeatureType* input_image_pipeline_feature =
      input_image_pipeline->mutable_type()->mutable_imagetype();
  input_image_pipeline_feature->set_width(image_width);
  input_image_pipeline_feature->set_height(image_height);
  input_image_pipeline_feature->set_colorspace(ImageFeatureType::RGB);

  // Write FeatureDescription for the IOU Threshold input.
  FeatureDescription* input_iou_pipeline = pipeline_desc->add_input();
  input_iou_pipeline->set_name("iouThreshold");
  input_iou_pipeline->set_shortdescription("(optional) IOU Threshold override (default: 0.45)");
  input_iou_pipeline->mutable_type()->mutable_doubletype();

  // Write FeatureDescription for the Confidence Threshold input.
  FeatureDescription* input_cthreshold_pipeline = pipeline_desc->add_input();
  input_cthreshold_pipeline->set_name("confidenceThreshold");
  input_cthreshold_pipeline->set_shortdescription(
    "(optional) Confidence Threshold override (default: 0.25)");
  input_cthreshold_pipeline->mutable_type()->mutable_doubletype();

  // Write FeatureDescription for the Confidence output.
  FeatureDescription* output_confidence_pipeline = pipeline_desc->add_output();
  output_confidence_pipeline->set_name("confidence");
  output_confidence_pipeline->set_shortdescription(
        "Boxes × Class confidence (see user-defined metadata \"classes\")");
  ArrayFeatureType* output_confidence_pipeline_feature =
      output_confidence_pipeline->mutable_type()->mutable_multiarraytype();
  output_confidence_pipeline_feature->set_datatype(ArrayFeatureType::DOUBLE);
  auto *confidence_shape_pipeline1 = output_confidence_pipeline_feature->mutable_shaperange()->add_sizeranges();
  confidence_shape_pipeline1->set_upperbound(-1);
  auto *confidence_shape_pipeline2 = output_confidence_pipeline_feature->mutable_shaperange()->add_sizeranges();
  confidence_shape_pipeline2->set_lowerbound(6);
  confidence_shape_pipeline2->set_upperbound(6);

  // Write FeatureDescription for the Coordinates output.
  FeatureDescription* output_coordinates_pipeline = pipeline_desc->add_output();
  output_coordinates_pipeline->set_name("coordinates");
  output_coordinates_pipeline->set_shortdescription(
        "Boxes × [x, y, width, height] (relative to image size)");
  ArrayFeatureType* output_coordinates_pipeline_feature =
      output_coordinates_pipeline->mutable_type()->mutable_multiarraytype();
  output_coordinates_pipeline_feature->set_datatype(ArrayFeatureType::DOUBLE);
  auto *coordinates_shape_pipeline1 = output_coordinates_pipeline_feature->mutable_shaperange()->add_sizeranges();
  coordinates_shape_pipeline1->set_upperbound(-1);
  auto *coordinates_shape_pipeline2 = output_coordinates_pipeline_feature->mutable_shaperange()->add_sizeranges();
  coordinates_shape_pipeline2->set_lowerbound(4);
  coordinates_shape_pipeline2->set_upperbound(4);
  
  // Add NeuralNetwork model to pipeline
  auto* model1 = model_pipeline.mutable_pipeline()->add_models();
  *model1 = model_nn;

  // Add Non Maximum Suppression model to pipeline
  auto* model2 = model_pipeline.mutable_pipeline()->add_models();
  *model2 = model_nms;

  // Wrap the pipeline
  auto pipeline_wrapper = std::make_shared<MLModelWrapper>(
    std::make_shared<CoreML::Model>(model_pipeline));
  
  // Add metadata.
  pipeline_wrapper->add_metadata({{ "user_defined", std::move(user_defined_metadata)}});

  return pipeline_wrapper;
}

}  // namespace turi
