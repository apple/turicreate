/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/coreml_export/neural_net_models_exporter.hpp>

#include <logger/assertions.hpp>
#include <unity/toolkits/coreml_export/mlmodel_include.hpp>
#include <util/string_util.hpp>

using CoreML::Specification::ArrayFeatureType;
using CoreML::Specification::FeatureDescription;
using CoreML::Specification::ImageFeatureType;
using CoreML::Specification::ModelDescription;
using CoreML::Specification::NeuralNetworkLayer;
using turi::coreml::MLModelWrapper;

  
namespace turi {

namespace {
  
constexpr char CONFIDENCE_STR[] = "Boxes × Class confidence (see user-defined metadata \"classes\")";
constexpr char COORDINATES_STR[] = "Boxes × [x, y, width, height] (relative to image size)";
constexpr char IOU_THRESHOLD_STR[] = "(optional) IOU Threshold override (default: 0.45)";
constexpr char CONFIDENCE_THRESHOLD_STR[] = "(optional) Confidence Threshold override (default: 0.25)";


void set_string_feature(FeatureDescription* feature_desc, std::string name,
                        std::string short_description)
{
  feature_desc->set_name(std::move(name));
  feature_desc->set_shortdescription(std::move(short_description));
  feature_desc->mutable_type()->mutable_stringtype();
}

void set_array_feature(FeatureDescription* feature_desc, std::string name,
                       std::string short_description,
                       const std::vector<size_t>& shape)
{
  // Set string values.
  feature_desc->set_name(std::move(name));
  feature_desc->set_shortdescription(std::move(short_description));

  // Set shape.
  ArrayFeatureType* array =
      feature_desc->mutable_type()->mutable_multiarraytype();
  for (size_t s : shape) {
    array->add_shape(s);
  }

  // Set data type.
  array->set_datatype(ArrayFeatureType::DOUBLE);
}

void set_dictionary_string_feature(FeatureDescription* feature_desc,
                                   std::string name,
                                   std::string short_description)
{
  feature_desc->set_name(std::move(name));
  feature_desc->set_shortdescription(std::move(short_description));
  feature_desc
    ->mutable_type()
    ->mutable_dictionarytype()
    ->mutable_stringkeytype();
}

void set_feature_optional(FeatureDescription* feature_desc) {
  feature_desc->mutable_type()->set_isoptional(true);
}

void set_predictions_feature(FeatureDescription* feature_desc, std::string feature_name, 
    size_t num_predictions, size_t num_classes, bool include_shape, bool use_flexible_shape,
    std::string short_desc) {
  feature_desc->set_name(feature_name);
  
  if (!short_desc.empty())
    feature_desc->set_shortdescription(short_desc);
  ArrayFeatureType* feature_desc_feature =
      feature_desc->mutable_type()->mutable_multiarraytype();
  if (include_shape) {
    feature_desc_feature->add_shape(num_predictions);
    feature_desc_feature->add_shape(num_classes);
  }
  feature_desc_feature->set_datatype(ArrayFeatureType::DOUBLE);

  if (use_flexible_shape) {
    auto *shape1 = feature_desc_feature->mutable_shaperange()
        ->add_sizeranges();
    shape1->set_upperbound(-1);
    auto *shape2 = feature_desc_feature->mutable_shaperange()
        ->add_sizeranges();
    shape2->set_lowerbound(num_classes);
    shape2->set_upperbound(num_classes);
  }
}

void set_threshold_feature(FeatureDescription* feature_desc, std::string feature_name, 
    std::string short_desc) {
  feature_desc->set_name(feature_name);
  if (!short_desc.empty())
    feature_desc->set_shortdescription(short_desc);
  feature_desc->mutable_type()->mutable_doubletype();
}

void set_image_feature(FeatureDescription* feature_desc, size_t image_width, 
    size_t image_height, bool include_description) {
  feature_desc->set_name("image");
  if (include_description)
    feature_desc->set_shortdescription("Input image");
  ImageFeatureType* image_feature =
      feature_desc->mutable_type()->mutable_imagetype();
  image_feature->set_width(image_width);
  image_feature->set_height(image_height);
  image_feature->set_colorspace(ImageFeatureType::RGB);
}

} //namespace

std::shared_ptr<MLModelWrapper> export_object_detector_model(
    const neural_net::model_spec& nn_spec, size_t image_width,
    size_t image_height, size_t num_classes, size_t num_predictions,
    flex_dict user_defined_metadata, flex_list class_labels, 
    std::map<std::string, flexible_type> options) {

  // Set up Pipeline 
  CoreML::Specification::Model model_pipeline;
  model_pipeline.set_specificationversion(3); 
  ModelDescription* pipeline_desc = model_pipeline.mutable_description();

  // Add NeuralNetwork model to pipeline
  auto* model_nn = model_pipeline.mutable_pipeline()->add_models();

  // Scale pixel values 0..255 to [0,1]
  NeuralNetworkLayer* first_layer =
      model_nn->mutable_neuralnetwork()->add_layers();
  first_layer->set_name("_divscalar0");
  first_layer->add_input("image");
  first_layer->add_output("_divscalar0");
  first_layer->mutable_scale()->add_shapescale(1);
  first_layer->mutable_scale()->mutable_scale()->add_floatvalue(1 / 255.f);

  // Copy the NeuralNetwork layers from nn_spec.
  // TODO: This copies ~60MB at present. Should object_detector be responsible
  // for _divscalar0, even though this isn't performed by the cnn_module?
  model_nn->mutable_neuralnetwork()->MergeFrom(nn_spec.get_coreml_spec());
  ASSERT_GT(model_nn->neuralnetwork().layers_size(), 1);

  // Wire up the input layer from the copied layers to _divscalar0.
  // TODO: This assumes that the first copied layer is the (only) one to take
  // the input from "image".
  NeuralNetworkLayer* second_layer = 
      model_nn->mutable_neuralnetwork()->mutable_layers(1);
  ASSERT_EQ(second_layer->input_size(), 1);
  ASSERT_EQ(second_layer->input(0), "image");
  second_layer->set_input(0, "_divscalar0");

  // Write the ModelDescription.
  ModelDescription* model_desc = model_nn->mutable_description();

  // Write FeatureDescription for the image input.
  set_image_feature(model_desc->add_input(), image_width, image_height, false);

  if (!options["include_non_maximum_suppression"].to<bool>()){
    
    // Write FeatureDescription for the confidence output.
    set_predictions_feature(model_desc->add_output(), "confidence", num_predictions, num_classes, 
      true, false, CONFIDENCE_STR);

    // Write FeatureDescription for the coordinates output.
    set_predictions_feature(model_desc->add_output(), "coordinates", num_predictions, 4, true, 
      false, COORDINATES_STR);

    // Set CoreML spec version.
    model_nn->set_specificationversion(1);
    auto model_wrapper = std::make_shared<MLModelWrapper>(
      std::make_shared<CoreML::Model>(*model_nn));

    // Add metadata.
    model_wrapper->add_metadata({
        { "user_defined", std::move(user_defined_metadata) }
    });

    return model_wrapper;
  }
  
  model_nn->set_specificationversion(3);

  // Write FeatureDescription for the raw confidence output.
  set_predictions_feature(model_desc->add_output(), "raw_confidence", num_predictions, num_classes, 
    true, true, "");

  // Write FeatureDescription for the coordinates output.
  set_predictions_feature(model_desc->add_output(), "raw_coordinates", num_predictions, 4, true, 
    true, "");

  // Add Non Maximum Suppression model to pipeline
  auto* model_nms = model_pipeline.mutable_pipeline()->add_models();
  model_nms->set_specificationversion(3);

  ModelDescription* nms_desc = model_nms->mutable_description();

  // Write FeatureDescription for the Raw Confidence input.
  set_predictions_feature(nms_desc->add_input(), "raw_confidence", num_predictions, num_classes, 
    true, true, "");

  // Write FeatureDescription for the Raw Coordinates input.
  set_predictions_feature(nms_desc->add_input(), "raw_coordinates", num_predictions, 4, 
    true, true, "");

  // Write FeatureDescription for the IOU Threshold input.
  set_threshold_feature(nms_desc->add_input(), "iouThreshold", "");

  // Write FeatureDescription for the Confidence Threshold input.
  set_threshold_feature(nms_desc->add_input(), "confidenceThreshold", "");

  // Write FeatureDescription for the Confidence output.
  set_predictions_feature(nms_desc->add_output(), "confidence", num_predictions, num_classes, 
    false, true, CONFIDENCE_STR);

  // Write FeatureDescription for the Coordinates input.
  set_predictions_feature(nms_desc->add_output(), "coordinates", num_predictions, 4, 
    false, true, COORDINATES_STR);
  
  CoreML::Specification::NonMaximumSuppression* first_layer_nms =
    model_nms->mutable_nonmaximumsuppression();
  
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

  // Write FeatureDescription for the image input.
  set_image_feature(pipeline_desc->add_input(), image_width, image_height, true);

  // Write FeatureDescription for the IOU Threshold input.
  set_threshold_feature(pipeline_desc->add_input(), "iouThreshold", IOU_THRESHOLD_STR);
  
  // Write FeatureDescription for the Confidence Threshold input.
  set_threshold_feature(pipeline_desc->add_input(), "confidenceThreshold", CONFIDENCE_THRESHOLD_STR);

  // Write FeatureDescription for the Confidence output.
  set_predictions_feature(pipeline_desc->add_output(), "confidence", num_predictions, num_classes, 
    false, true, CONFIDENCE_STR);

  // Write FeatureDescription for the Coordinates output.
  set_predictions_feature(pipeline_desc->add_output(), "coordinates", num_predictions, 4, 
    false, true, COORDINATES_STR);

  
  // Wrap the pipeline
  auto pipeline_wrapper = std::make_shared<MLModelWrapper>(
    std::make_shared<CoreML::Model>(model_pipeline));
  
  // Add metadata.
  pipeline_wrapper->add_metadata({{ "user_defined", std::move(user_defined_metadata)}});

  return pipeline_wrapper;
}

std::shared_ptr<MLModelWrapper> export_activity_classifier_model(
    const neural_net::model_spec& nn_spec, size_t prediction_window,
    const flex_list& features, size_t lstm_hidden_layer_size,
    const flex_list& class_labels, const flex_string& target)
{
  CoreML::Specification::Model model;
  model.set_specificationversion(1);

  // Write the model description.
  ModelDescription* model_desc = model.mutable_description();

  // Write the primary input features.
  for (size_t i = 0; i < features.size(); i++) {
    set_array_feature(model_desc->add_input(), features[i],
                      flex_string(features[i]) + " window input",
                      {prediction_window});
  }

  // Write the primary output features.
  set_dictionary_string_feature(model_desc->add_output(),
                                target + "Probability",
                                "Activity prediction probabilities");

  set_string_feature(model_desc->add_output(), target,
                     "Class label of top prediction");

  // Write the (optional) LSTM input and output features.
  FeatureDescription* feature_desc = model_desc->add_input();
  set_array_feature(feature_desc, "stateIn", "LSTM state input",
                    { lstm_hidden_layer_size*2 });

  set_feature_optional(feature_desc);
  set_array_feature(model_desc->add_output(), "stateOut",
                    "LSTM state output", { lstm_hidden_layer_size * 2 });

  // Specify the prediction output names.
  model_desc->set_predictedfeaturename(target);
  model_desc->set_predictedprobabilitiesname(target + "Probability");

  // Write the neural network.
  CoreML::Specification::NeuralNetworkClassifier* nn_classifier =
      model.mutable_neuralnetworkclassifier();

  // Copy the layers and preprocessing from the provided spec.
  nn_classifier->mutable_layers()->CopyFrom(nn_spec.get_coreml_spec().layers());
  if (nn_spec.get_coreml_spec().preprocessing_size() > 0) {
    nn_classifier->mutable_preprocessing()->CopyFrom(
        nn_spec.get_coreml_spec().preprocessing());
  }

  // Add the classifier fields: class labels and probability output name.
  for (const auto& class_label : class_labels) {
    nn_classifier->mutable_stringclasslabels()->add_vector(
        class_label.to<flex_string>());
  }
  nn_classifier->set_labelprobabilitylayername(target + "Probability");

  return std::make_shared<MLModelWrapper>(
      std::make_shared<CoreML::Model>(model));
}

}  // namespace turi
