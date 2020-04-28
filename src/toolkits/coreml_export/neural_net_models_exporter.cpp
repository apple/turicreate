/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/coreml_export/neural_net_models_exporter.hpp>

#include <locale>
#include <sstream>

#include <core/logging/assertions.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <core/util/string_util.hpp>

using CoreML::Specification::ArrayFeatureType;
using CoreML::Specification::FeatureDescription;
using CoreML::Specification::ImageFeatureType;
using CoreML::Specification::ImageFeatureType_ImageSizeRange;
using CoreML::Specification::ModelDescription;
using CoreML::Specification::NeuralNetworkLayer;
using CoreML::Specification::NeuralNetworkPreprocessing;
using CoreML::Specification::SizeRange;
using turi::coreml::MLModelWrapper;


namespace turi {

namespace {

constexpr char CONFIDENCE_STR[] = "Boxes × Class confidence (see user-defined metadata \"classes\")";
constexpr char COORDINATES_STR[] = "Boxes × [x, y, width, height] (relative to image size)";

std::string iou_threshold_description(float default_value) {
  std::stringstream ss;
  ss << "The maximum allowed overlap (as intersection-over-union ratio) for any"
     << " pair of output bounding boxes (default: " << default_value << ")";
  return ss.str();
}

std::string confidence_threshold_description(float default_value) {
  std::stringstream ss;
  ss << "The minimum confidence score for an output bounding box"
     << " (default: " << default_value << ")";
  return ss.str();
}

void set_string_feature(FeatureDescription* feature_desc, std::string name,
                        std::string short_description)
{
  feature_desc->set_name(std::move(name));
  feature_desc->set_shortdescription(std::move(short_description));
  feature_desc->mutable_type()->mutable_stringtype();
}

void set_int64_feature(FeatureDescription* feature_desc, std::string name,
                       std::string short_description)
{
  feature_desc->set_name(std::move(name));
  feature_desc->set_shortdescription(std::move(short_description));
  feature_desc->mutable_type()->mutable_int64type();
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

void set_image_feature_size_range(ImageFeatureType* image_feature,
                                  int64_t width_lower,
                                  int64_t width_higher,
                                  int64_t height_lower,
                                  int64_t height_higher) {
  ImageFeatureType_ImageSizeRange* image_size_range =
      image_feature->mutable_imagesizerange();

  SizeRange* width_range = image_size_range->mutable_widthrange();
  SizeRange* height_range = image_size_range->mutable_heightrange();

  width_range->set_lowerbound(width_lower);
  width_range->set_upperbound(width_higher);

  height_range->set_lowerbound(height_lower);
  height_range->set_upperbound(height_higher);
}

ImageFeatureType* set_image_feature(
    FeatureDescription* feature_desc, size_t image_width, size_t image_height,
    std::string input_name, std::string description = "",
    ImageFeatureType::ColorSpace image_type = ImageFeatureType::RGB) {
  feature_desc->set_name(input_name);
  if (!description.empty()) feature_desc->set_shortdescription(description);

  ImageFeatureType* image_feature =
      feature_desc->mutable_type()->mutable_imagetype();
  image_feature->set_width(image_width);
  image_feature->set_height(image_height);
  image_feature->set_colorspace(image_type);

  return image_feature;
}

} //namespace

std::shared_ptr<MLModelWrapper> export_object_detector_model(
    neural_net::pipeline_spec raw_pipeline, size_t num_classes,
    size_t num_predictions, flex_list class_labels,
    std::map<std::string, flexible_type> options) {
  // Set up Pipeline
  CoreML::Specification::Model model_pipeline;
  model_pipeline.set_specificationversion(3);
  ModelDescription* pipeline_desc = model_pipeline.mutable_description();

  // Adopt the model pipeline passed to us as input.
  std::unique_ptr<CoreML::Specification::Pipeline> raw_pipeline_spec =
      std::move(raw_pipeline).move_coreml_spec();
  model_pipeline.mutable_pipeline()->Swap(raw_pipeline_spec.get());

  if (!options["include_non_maximum_suppression"].to<bool>()){
    // Only support this case for models supporting spec version 1, which means
    // no pipeline models.
    ASSERT_EQ(model_pipeline.pipeline().models_size(), 1);

    auto model_wrapper = std::make_shared<MLModelWrapper>(
        std::make_shared<CoreML::Model>(model_pipeline.pipeline().models(0)));

    return model_wrapper;
  }

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

  // Copy input feature descriptions from the first model in the pipeline.
  *pipeline_desc->mutable_input() =
      model_pipeline.pipeline().models(0).description().input();

  // Write FeatureDescription for the IOU Threshold input.
  FeatureDescription* iou_threshold = pipeline_desc->add_input();
  set_threshold_feature(iou_threshold, "iouThreshold",
                        iou_threshold_description(options["iou_threshold"]));
  set_feature_optional(iou_threshold);

  // Write FeatureDescription for the Confidence Threshold input.
  FeatureDescription* confidence_threshold = pipeline_desc->add_input();
  set_threshold_feature(
      confidence_threshold, "confidenceThreshold",
      confidence_threshold_description(options["confidence_threshold"]));
  set_feature_optional(confidence_threshold);

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

std::shared_ptr<coreml::MLModelWrapper> export_style_transfer_model(
    const neural_net::model_spec& nn_spec, size_t image_width,
    size_t image_height, bool include_flexible_shape,
    std::string content_feature, std::string style_feature, size_t num_styles) {

  CoreML::Specification::Model model;
  model.set_specificationversion(3);

  ModelDescription* model_desc = model.mutable_description();

  FeatureDescription* model_input = model_desc->add_input();
  ImageFeatureType* input_feat = set_image_feature(model_input,
                                                   image_width,
                                                   image_height,
                                                   content_feature,
                                                   "Input image");

  set_array_feature(
      model_desc->add_input(), "index",
      "Style index array (set index I to 1.0 to enable Ith style)", {num_styles});
  /*
   * prefix style with stylized and capitalize the following identifier, this
   * avoids name clashes with the `content_feature` for exporting to CoreML.
   */
  style_feature[0] = std::toupper(style_feature[0]);
  style_feature = "stylized" + style_feature;

  FeatureDescription* model_output = model_desc->add_output();
  ImageFeatureType* style_feat = set_image_feature(model_output,
                                                   image_width,
                                                   image_height,
                                                   style_feature,
                                                   "Stylized image");

  /**
   * The -1 indicates no upper limits for the image size
   */
  if (include_flexible_shape) {
    set_image_feature_size_range(input_feat, 64, -1, 64, -1);
    set_image_feature_size_range(style_feat, 64, -1, 64, -1);
  }

  CoreML::Specification::NeuralNetwork* nn = model.mutable_neuralnetwork();
  nn->MergeFrom(nn_spec.get_coreml_spec());

  /*
    Change input to first and last layers to match input and output feature names.
  */
  int last_layer_index = nn->layers_size() - 1;
  NeuralNetworkLayer* first_layer = nn->mutable_layers(0);
  NeuralNetworkLayer* last_layer = nn->mutable_layers(last_layer_index);

  first_layer->set_input(0, content_feature);
  last_layer->set_output(0, style_feature);

  auto model_wrapper =
      std::make_shared<MLModelWrapper>(std::make_shared<CoreML::Model>(model));

  return model_wrapper;
}

std::shared_ptr<coreml::MLModelWrapper> export_drawing_classifier_model(
    const neural_net::model_spec& nn_spec, const flex_list& features,
    const flex_list& class_labels, const flex_string& target)
{
  CoreML::Specification::Model model;
  model.set_specificationversion(1);

  // Write the model description.
  ModelDescription* model_desc = model.mutable_description();

  // Write the primary input features.
  for (size_t i = 0; i < features.size(); i++) {
    set_image_feature(model_desc->add_input(), /* W */ 28, /* H */ 28,
                      features[i].to<flex_string>(), "Input image",
                      ImageFeatureType::GRAYSCALE);
  }

  // Write the primary output features.
  set_dictionary_string_feature(model_desc->add_output(),
                                target + "Probability",
                                "drawing classifier prediction probabilities");

  flex_type_enum class_type = class_labels.begin()->get_type();

  if (class_type == flex_type_enum::STRING) {
    set_string_feature(model_desc->add_output(), target,
                       "drawing classifier class label of top prediction");
  } else {
    set_int64_feature(model_desc->add_output(), target,
                      "drawing classifier class label of top prediction");
  }

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

    if (class_type == flex_type_enum::STRING) {
      nn_classifier->mutable_stringclasslabels()->add_vector(
        class_label.to<flex_string>());
    } else {
      nn_classifier->mutable_int64classlabels()->add_vector(
        class_label.to<flex_int>());
    }

  }

  nn_classifier->set_labelprobabilitylayername(target + "Probability");

  return std::make_shared<MLModelWrapper>(
      std::make_shared<CoreML::Model>(model));
}

}  // namespace turi
