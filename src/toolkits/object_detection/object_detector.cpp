/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/object_detection/object_detector.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <sstream>
#include <utility>
#include <vector>

#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <core/random/random.hpp>
#include <timer/timer.hpp>
#include <toolkits/coreml_export/neural_net_models_exporter.hpp>
#include <toolkits/object_detection/od_darknet_yolo_model_trainer.hpp>
#include <toolkits/object_detection/od_evaluation.hpp>
#include <toolkits/object_detection/od_serialization.hpp>
#include <toolkits/object_detection/od_yolo.hpp>
#include <toolkits/supervised_learning/automatic_model_creation.hpp>
#include <toolkits/util/training_utils.hpp>

#ifdef __APPLE__

#include <os/log.h>

static const os_log_t& _get_os_log_object() {
  static_assert(std::is_trivially_destructible<os_log_t>::value,
                "static variables should never de-initialize");
  static os_log_t log_object = os_log_create("com.apple.turi",
                                             "object_detector");
  return log_object;
}

#define _os_log_integer(key, value)                                            \
    os_log_info(_get_os_log_object(),                                          \
                "event: %lu, key: %s, value: %ld",                             \
                1ul, (key), static_cast<long>(value))

#else  // #ifdef __APPLE__

#define _os_log_integer(...)

#endif  // #ifdef __APPLE__

using turi::coreml::MLModelWrapper;
using turi::neural_net::compute_context;
using turi::neural_net::deferred_float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::FuturesStream;
using turi::neural_net::image_annotation;
using turi::neural_net::image_augmenter;
using turi::neural_net::labeled_image;
using turi::neural_net::model_backend;
using turi::neural_net::model_spec;
using turi::neural_net::shared_float_array;
using turi::neural_net::xavier_weight_initializer;

using padding_type = model_spec::padding_type;

namespace turi {
namespace object_detection {

namespace {

constexpr size_t OBJECT_DETECTOR_VERSION = 1;

constexpr int DEFAULT_BATCH_SIZE = 32;

// Empircally, we need 4GB to support batch size 32.
constexpr size_t MEMORY_REQUIRED_FOR_DEFAULT_BATCH_SIZE = 4294967296;

// The spatial reduction depends on the input size of the pre-trained model
// (relative to the grid size).
// TODO: When we support alternative base models, we will have to generalize.
constexpr int SPATIAL_REDUCTION = 32;

constexpr float DEFAULT_NON_MAXIMUM_SUPPRESSION_THRESHOLD = 0.45f;

constexpr float DEFAULT_CONFIDENCE_THRESHOLD_PREDICT = 0.25f;

constexpr float DEFAULT_CONFIDENCE_THRESHOLD_EVALUATE = 0.001f;

// before generation precision-recall curves.

// Each bounding box is evaluated relative to a list of pre-defined sizes.
const std::vector<std::pair<float, float>>& anchor_boxes() {
  static const std::vector<std::pair<float, float>>* const default_boxes =
      new std::vector<std::pair<float, float>>({
          {1.f, 2.f}, {1.f, 1.f}, {2.f, 1.f},
          {2.f, 4.f}, {2.f, 2.f}, {4.f, 2.f},
          {4.f, 8.f}, {4.f, 4.f}, {8.f, 4.f},
          {8.f, 16.f}, {8.f, 8.f}, {16.f, 8.f},
          {16.f, 32.f}, {16.f, 16.f}, {32.f, 16.f},
      });
  return *default_boxes;
};

flex_int estimate_max_iterations(flex_int num_instances, flex_int batch_size) {

  // Scale with square root of number of labeled instances.
  float num_images = 5000.f * std::sqrt(static_cast<float>(num_instances));

  // Normalize by batch size.
  float num_iter_raw = num_images / batch_size;

  // Round to the nearest multiple of 1000.
  float num_iter_rounded = 1000.f * std::round(num_iter_raw / 1000.f);

  // Always return a positive number.
  return std::max(1000, static_cast<int>(num_iter_rounded));
}

}  // namespace

void object_detector::init_options(
    const std::map<std::string, flexible_type>& opts) {

  // Define options.
  options.create_integer_option(
      "batch_size",
      "The number of images to process for each training iteration",
      FLEX_UNDEFINED,
      1,
      std::numeric_limits<int>::max());
  options.create_integer_option(
      "max_iterations",
      "Maximum number of iterations to perform during training",
      FLEX_UNDEFINED,
      1,
      std::numeric_limits<int>::max());
  options.create_integer_option(
      /* name             */ "grid_height",
      /* description      */
      "Height of the grid of features computed for each image",
      /* default_value    */ 13,
      /* lower_bound      */ 1,
      /* upper_bound      */ std::numeric_limits<int>::max());
  options.create_integer_option(
      /* name             */ "grid_width",
      /* description      */
      "Width of the grid of features computed for each image",
      /* default_value    */ 13,
      /* lower_bound      */ 1,
      /* upper_bound      */ std::numeric_limits<int>::max());
  options.create_integer_option(
      "random_seed",
      "Seed for random weight initialization and sampling during training",
      FLEX_UNDEFINED,
      std::numeric_limits<int>::min(),
      std::numeric_limits<int>::max());
  options.create_categorical_option(
      /* name              */ "annotation_scale",
      /* description       */
      "Defines annotations scale: pixel or normalized",
      /* default_value     */ "pixel",
      /* allowed_values    */ {flexible_type("pixel"), flexible_type("normalized")},
      /* allowed_overwrite */ false);
  options.create_categorical_option(
      /* name              */ "annotation_origin",
      /* description       */
      "Defines image origin: top_left or bottom_left",
      /* default_value     */ "top_left",
      /* allowed_values    */ {flexible_type("top_left"), flexible_type("bottom_left")},
      /* allowed_overwrite */ false);
  options.create_categorical_option(
      /* name              */ "annotation_position",
      /* description       */
      "Defines annotations position: center, top_left or bottom_left",
      /* default_value     */ "center",
      /* allowed_values    */ {flexible_type("center"), flexible_type("top_left"), flexible_type("bottom_left")},
      /* allowed_overwrite */ false);
  options.create_flexible_type_option(
      /* name              */ "classes",
      /* description       */
      "Defines class labels.",
      /* default_value     */ flex_list(),
      /* allowed_overwrite */ false);
  options.create_boolean_option(
      "verbose",
      "If True, print progress updates and model details.",
      true,
      true);
  options.create_string_option(
      /* name              */ "model",
      /* description       */
      "Defines the model type",
      /* default_value     */ "darknet-yolo",
      /* allowed_overwrite */ true);

  // Validate user-provided options.
  options.set_options(opts);

  // Write model fields.
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

void object_detector::infer_derived_options(compute_context* context,
                                            data_iterator* iterator) {
  context->print_training_device_info();

  // Configure the batch size automatically if not set.
  if (read_state<flexible_type>("batch_size") == FLEX_UNDEFINED) {

    flex_int batch_size = DEFAULT_BATCH_SIZE;
    size_t memory_budget = context->memory_budget();
    if (memory_budget < MEMORY_REQUIRED_FOR_DEFAULT_BATCH_SIZE) {
      batch_size /= 2;
    }
    // TODO: What feedback can we give if the user requests a batch size that
    // doesn't fit?

    logprogress_stream << "Setting 'batch_size' to " << batch_size;

    add_or_update_state({{"batch_size", batch_size}});
  }
  _os_log_integer("batch_size", read_state<flex_int>("batch_size"));

  // Configure targeted number of iterations automatically if not set.
  if (read_state<flexible_type>("max_iterations") == FLEX_UNDEFINED) {
    flex_int max_iterations = estimate_max_iterations(
        static_cast<flex_int>(iterator->num_instances()),
        read_state<flex_int>("batch_size"));

    logprogress_stream << "Setting 'max_iterations' to " << max_iterations;

    add_or_update_state({{"max_iterations", max_iterations}});
  }
  _os_log_integer("max_iterations", read_state<flex_int>("max_iterations"));
}

size_t object_detector::get_version() const {
  return OBJECT_DETECTOR_VERSION;
}

void object_detector::save_impl(oarchive& oarc) const {
  const Checkpoint& checkpoint = read_checkpoint();
  _save_impl(oarc, state, checkpoint.weights());
}

void object_detector::load_version(iarchive& iarc, size_t version) {
  // First read from the archive into local variables for the state and model
  // weights.
  std::map<std::string, variant_type> loaded_state;
  float_array_map loaded_weights;
  _load_version(iarc, version, &loaded_state, &loaded_weights);

  // Adopt the loaded state and weights.
  load(std::move(loaded_state), std::move(loaded_weights));
}

void object_detector::load(std::map<std::string, variant_type> state,
                           float_array_map weights) {
  this->state = std::move(state);
  checkpoint_ = load_checkpoint(std::move(weights));
}

std::unique_ptr<Checkpoint> object_detector::load_checkpoint(
    float_array_map weights) const {
  // Write from the state into a new Config struct.
  Config config;
  config.max_iterations = static_cast<int>(get_max_iterations());
  config.batch_size = read_state<int>("batch_size");
  config.output_height = read_state<int>("grid_height");
  config.output_width = read_state<int>("grid_width");
  config.num_classes = static_cast<int>(get_num_classes());

  auto it = state.find("random_seed");
  if (it != state.end()) {
    config.random_seed = variant_get_value<int>(it->second);
  }

  std::unique_ptr<Checkpoint> checkpoint;
  checkpoint.reset(
      new DarknetYOLOCheckpoint(std::move(config), std::move(weights)));
  return checkpoint;
}

const Checkpoint& object_detector::read_checkpoint() const {
  if (checkpoint_ == nullptr) {
    checkpoint_ = std::move(*checkpoint_futures_->Next().get());
  }
  return *checkpoint_.get();
}

void object_detector::import_from_custom_model(variant_map_type model_data,
                                               size_t version) {
  auto model_iter = model_data.find("_model");
  if (model_iter == model_data.end()) {
    log_and_throw("The loaded turicreate model must contain '_model'!\n");
  }

  const flex_dict& model = variant_get_value<flex_dict>(model_iter->second);
  flex_dict mxnet_data_dict;
  flex_dict mxnet_shape_dict;

  for (const auto& data : model) {
    if (data.first == "data") {
      mxnet_data_dict = data.second;
    }
    if (data.first == "shapes") {
      mxnet_shape_dict = data.second;
    }
  }

  auto shape_iter = model_data.find("_grid_shape");
  size_t height, width;
  if (shape_iter == model_data.end()) {
    height = 13;
    width = 13;
  } else {
    std::vector<size_t> shape =
        variant_get_value<std::vector<size_t>>(shape_iter->second);
    height = shape[0];
    width = shape[1];
  }

  auto cmp = [](const flex_dict::value_type& a,
                const flex_dict::value_type& b) { return (a.first < b.first); };

  std::sort(mxnet_data_dict.begin(), mxnet_data_dict.end(), cmp);
  std::sort(mxnet_shape_dict.begin(), mxnet_shape_dict.end(), cmp);

  float_array_map nn_params;

  for (size_t i = 0; i < mxnet_data_dict.size(); i++) {
    std::string layer_name = mxnet_data_dict[i].first;
    flex_nd_vec mxnet_data_nd = mxnet_data_dict[i].second.to<flex_nd_vec>();
    flex_nd_vec mxnet_shape_nd = mxnet_shape_dict[i].second.to<flex_nd_vec>();
    const std::vector<double>& model_weight = mxnet_data_nd.elements();
    const std::vector<double>& model_shape = mxnet_shape_nd.elements();
    std::vector<float> layer_weight(model_weight.begin(), model_weight.end());
    std::vector<size_t> layer_shape(model_shape.begin(), model_shape.end());
    size_t index = layer_name.find('_');
    layer_name =
        layer_name.substr(0, index) + "_fwd_" + layer_name.substr(index + 1);
    nn_params[layer_name] = shared_float_array::wrap(std::move(layer_weight),
                                                     std::move(layer_shape));
  }

  // adding meta data
  model_data.emplace("grid_height", height);
  model_data.emplace("grid_width", width);
  model_data.emplace("annotation_scale", "pixel");
  model_data.emplace("annotation_origin", "top_left");
  model_data.emplace("annotation_position", "center");
  model_data.erase(model_iter);
  model_data.erase(shape_iter);

  load(std::move(model_data), std::move(nn_params));
}

void object_detector::train(gl_sframe data,
                            std::string annotations_column_name,
                            std::string image_column_name,
                            variant_type validation_data,
                            std::map<std::string, flexible_type> opts)
{
  auto compute_final_metrics_iter = opts.find("compute_final_metrics");
  bool compute_final_metrics = true;
  if (compute_final_metrics_iter != opts.end()) {
    compute_final_metrics = compute_final_metrics_iter->second;
    opts.erase(compute_final_metrics_iter);
  }

  // Instantiate the training dependencies: data iterator, image augmenter,
  // backend NN model.
  init_training(data, annotations_column_name, image_column_name,
                validation_data, opts);

  turi::timer time_object;
  time_object.start();

  // Perform all the iterations at once.
  while (get_training_iterations() < get_max_iterations()) {
    iterate_training();
  }

  // Wait for any outstanding batches to finish.
  finalize_training(compute_final_metrics);

  double current_time = time_object.current_time();

  std::stringstream ss;
  table_internal::_format_time(ss, current_time);

  add_or_update_state({
    {"training_time", current_time},
    {"_training_time_as_string", ss.str()}
  });
}

void object_detector::finalize_training(bool compute_final_metrics) {
  // Wait for any outstanding batches.
  synchronize_training();

  // Finish printing progress.
  if (training_table_printer_) {
    training_table_printer_->print_footer();
    training_table_printer_.reset();
  }

  // Copy out the trained model while we still have access to a backend.
  read_checkpoint();

  // Tear down the training backend.
  checkpoint_futures_.reset();
  training_futures_.reset();

  // Compute training and validation metrics.
  if (compute_final_metrics) {
    update_model_metrics(training_data_, validation_data_);
  }
}

variant_type object_detector::evaluate(gl_sframe data, std::string metric,
                                       std::string output_type,
                                       std::map<std::string, flexible_type> opts) {
  // check if data has ground truth annotation
  std::string annotations_column_name = read_state<flex_string>("annotations");
  if (!data.contains_column(annotations_column_name)) {
    log_and_throw("Annotations column " + annotations_column_name +
                  " does not exist");
  }

  //parse input opts
  float confidence_threshold, iou_threshold;
  auto it_confidence = opts.find("confidence_threshold");
  if (it_confidence == opts.end()){
    confidence_threshold = DEFAULT_CONFIDENCE_THRESHOLD_EVALUATE;
  } else {
    confidence_threshold = opts["confidence_threshold"];
  }
  auto it_iou = opts.find("iou_threshold");
  if (it_iou == opts.end()){
    iou_threshold = DEFAULT_NON_MAXIMUM_SUPPRESSION_THRESHOLD;
  } else {
    iou_threshold = opts["iou_threshold"];
  }

  std::vector<std::string> metrics;
  static constexpr char AP[] = "average_precision";
  static constexpr char MAP[] = "mean_average_precision";
  static constexpr char AP50[] = "average_precision_50";
  static constexpr char MAP50[] = "mean_average_precision_50";
  std::vector<std::string> all_metrics = {AP, MAP, AP50, MAP50};
  if (std::find(all_metrics.begin(), all_metrics.end(), metric) !=
      all_metrics.end()) {
    metrics = {metric};
  } else if (metric == "auto") {
    metrics = {AP50, MAP50};
  } else if (metric == "all" || metric == "report") {
    metrics = all_metrics;
  } else {
    log_and_throw("Metric " + metric + " not supported");
  }

  flex_list class_labels = read_state<flex_list>("classes");
  // Initialize the metric calculator
  average_precision_calculator calculator(class_labels);

  auto consumer = [&](const std::vector<image_annotation>& predicted_row,
                      const std::vector<image_annotation>& groundtruth_row,
                      const std::pair<float, float>& image_dimension) {
    calculator.add_row(predicted_row, groundtruth_row);
  };

  perform_predict(data, consumer, confidence_threshold, iou_threshold);

  // Compute the average precision (area under the precision-recall curve) for
  // each combination of IOU threshold and class label.
  variant_map_type result_map = calculator.evaluate();

  // Trim undesired metrics from the final result. (For consistency with other
  // toolkits. In this case, almost all of the work is shared across metrics.)
  if (std::find(metrics.begin(), metrics.end(), AP) == metrics.end()) {
    result_map.erase(AP);
  }
  if (std::find(metrics.begin(), metrics.end(), AP50) == metrics.end()) {
    result_map.erase(AP50);
  }
  if (std::find(metrics.begin(), metrics.end(), MAP50) == metrics.end()) {
    result_map.erase(MAP50);
  }
  if (std::find(metrics.begin(), metrics.end(), MAP) == metrics.end()) {
    result_map.erase(MAP);
  }

  return convert_map_to_types(result_map, output_type,
                              read_state<flex_list>("classes"));
}

variant_type object_detector::convert_map_to_types(
    const variant_map_type& result_map, const std::string& output_type,
    const flex_list& class_labels) {
  // Handle different output types here
  // If output_type = "dict", just return the result_map.
  // If output_type = "sframe", construct a sframe,
  // whose rows indicate class labels, and columns denote different metric
  // scores. Note that the "sframe" output only shows AP or AP50.

  variant_type final_result;
  std::string AP = "average_precision";
  std::string AP50 = "average_precision_50";

  if (output_type == "dict") {
    final_result = to_variant(result_map);
  } else if (output_type == "sframe") {
    gl_sframe sframe_result({{"label", gl_sarray(class_labels)}});
    auto add_score_list = [&](std::string& metric_name) {
      flex_list score_list;
      auto it = result_map.find(metric_name);
      if (it != result_map.end()) {
        const flex_dict& dict = variant_get_value<flex_dict>(it->second);
        for (const auto& label_score_pair : dict) {
          score_list.push_back(label_score_pair.second);
        }
        sframe_result.add_column(gl_sarray(score_list), metric_name);
      }
    };
    add_score_list(AP);
    add_score_list(AP50);
    final_result = to_variant(sframe_result);
  } else {
    log_and_throw(
        "Invalid 'output_type' argument! Only 'dict' and 'sframe' are "
        "accepted.");
  }

  return final_result;
}

variant_type object_detector::predict(
    variant_type data, std::map<std::string, flexible_type> opts) {
  gl_sarray_writer result(flex_type_enum::LIST, 1);

  auto consumer = [&](const std::vector<image_annotation>& predicted_row,
                      const std::vector<image_annotation>& groundtruth_row,
                      const std::pair<float, float>& image_dimension) {
    // Convert predicted_row to flex_type list to call gl_sarray_writer
    flex_list predicted_row_ft;
    flex_list class_labels = read_state<flex_list>("classes");
    float height_scale = image_dimension.first;
    float width_scale = image_dimension.second;
    for (size_t i = 0; i < predicted_row.size(); i++) {
      const image_annotation& each_row = predicted_row[i];

      flex_dict bb_dict = {
          {"x", (each_row.bounding_box.x + each_row.bounding_box.width / 2.) *
                    width_scale},
          {"y", (each_row.bounding_box.y + each_row.bounding_box.height / 2.) *
                    height_scale},
          {"width", each_row.bounding_box.width * width_scale},
          {"height", each_row.bounding_box.height * height_scale}};

      flex_dict each_annotation = {
          {"label", class_labels[each_row.identifier].to<flex_string>()},
          {"type", "rectangle"},
          {"coordinates", std::move(bb_dict)},
          {"confidence", each_row.confidence}};
      predicted_row_ft.push_back(std::move(each_annotation));
    }
    result.write(predicted_row_ft, 0);
  };
  // Parse input options
  float confidence_threshold, iou_threshold;
  auto it_confidence = opts.find("confidence_threshold");
  if (it_confidence == opts.end()){
    confidence_threshold = DEFAULT_CONFIDENCE_THRESHOLD_PREDICT;
  } else {
    confidence_threshold = opts["confidence_threshold"];
  }
  auto it_iou = opts.find("iou_threshold");
  if (it_iou == opts.end()){
    iou_threshold = DEFAULT_NON_MAXIMUM_SUPPRESSION_THRESHOLD;
  } else {
    iou_threshold = opts["iou_threshold"];
  }

  // Convert data to SFrame
  std::string image_column_name = read_state<flex_string>("feature");
  gl_sframe sframe_data = convert_types_to_sframe(data, image_column_name);

  // Predict function should only depends on the feature column
  // So we extract the image column only.
  if (!sframe_data.contains_column(image_column_name)) {
    log_and_throw("Column name '" + image_column_name + "' does not exist.");
  }
  gl_sframe sframe_image_data(
      {{image_column_name, sframe_data[image_column_name]}});

  perform_predict(sframe_image_data, consumer, confidence_threshold,
                  iou_threshold);

  // Convert output to flex_list if data is a single image
  gl_sarray result_sarray = result.close();
  variant_type final_result;
  if (variant_is<gl_sframe>(data) || variant_is<gl_sarray>(data)) {
    final_result = to_variant(result_sarray);
  } else {
    final_result = to_variant(result_sarray[0]);
  }
  return final_result;
}

gl_sframe object_detector::convert_types_to_sframe(
    const variant_type& data, const std::string& column_name) {
  // Data input can be either sframe, sarray, or a single image
  // If they are sarray or image, create a sframe with a single column.
  gl_sframe sframe_data;
  if (variant_is<gl_sframe>(data)) {
    sframe_data = variant_get_value<gl_sframe>(data);
  } else if (variant_is<flexible_type>(data)) {
    flexible_type image_data = variant_get_value<flexible_type>(data);
    std::vector<flexible_type> image_vector {image_data};
    std::map<std::string, std::vector<flexible_type>> image_map = {
        {column_name, image_vector}};
    sframe_data = gl_sframe(image_map);
  } else if (variant_is<gl_sarray>(data)){
    gl_sarray sarray_data = variant_get_value<gl_sarray>(data);
    std::map<std::string, gl_sarray> sarray_map = {{column_name, sarray_data}};
    sframe_data = gl_sframe(sarray_map);
  } else {
    log_and_throw("Invalid data type for predict()! Expect Sframe, Sarray, or flexible_type!");
  }
  return sframe_data;
}

std::unique_ptr<ModelTrainer> object_detector::create_inference_trainer(
    const Checkpoint& checkpoint,
    std::unique_ptr<neural_net::compute_context> context) const {
  return checkpoint.CreateModelTrainer(context.get());
}

void object_detector::perform_predict(
    gl_sframe data,
    std::function<void(const std::vector<image_annotation>&,
                       const std::vector<image_annotation>&,
                       const std::pair<float, float>&)>
        consumer,
    float confidence_threshold, float iou_threshold) {
  flex_list class_labels = read_state<flex_list>("classes");
  int batch_size = read_state<int>("batch_size");

  // return if the data is empty
  if (data.size() == 0) return;

  // Bind the data to a data iterator.
  std::unique_ptr<data_iterator> data_iter = create_iterator(
      data, std::vector<std::string>(class_labels.begin(), class_labels.end()),
      /* repeat */ false, /* is_training*/ false);

  // Instantiate the compute context.
  std::unique_ptr<compute_context> ctx = create_compute_context();
  if (ctx == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  // Construct a pipeline generating inference results.
  std::unique_ptr<ModelTrainer> model_trainer =
      create_inference_trainer(read_checkpoint(), std::move(ctx));
  std::shared_ptr<FuturesStream<EncodedBatch>> inference_futures =
      model_trainer
          ->AsInferenceBatchPublisher(std::move(data_iter), batch_size,
                                      confidence_threshold, iou_threshold)
          ->AsFutures();

  // Consume the results, ensuring that we have the next batch in progress in
  // the background while we consume the previous batch.
  std::future<std::unique_ptr<EncodedBatch>> pending_batch =
      inference_futures->Next();
  while (pending_batch.valid()) {
    // Start the next batch before we handle the pending batch.
    std::future<std::unique_ptr<EncodedBatch>> next_batch =
        inference_futures->Next();

    // Wait for the pending batch to be complete.
    std::unique_ptr<EncodedBatch> encoded_batch = pending_batch.get();
    if (encoded_batch) {
      // We have more raw results. Decode them.
      InferenceOutputBatch batch = model_trainer->DecodeOutputBatch(
          *encoded_batch, confidence_threshold, iou_threshold);

      // Consume the results.
      for (size_t i = 0; i < batch.annotations.size(); ++i) {
        consumer(batch.predictions[i], batch.annotations[i],
                 batch.image_sizes[i]);
      }

      // Continue iterating.
      pending_batch = std::move(next_batch);
    }
  }
}

// TODO: Should accept model_backend as an optional argument to avoid
// instantiating a new backend during training. Or just check to see if an
// existing backend is available?
variant_type object_detector::perform_evaluation(gl_sframe data,
                                                 std::string metric,
                                                 std::string output_type,
                                                 float confidence_threshold,
                                                 float iou_threshold) {
  std::map<std::string, flexible_type> opts{{"confidence_threshold", confidence_threshold},
{"iou_threshold", iou_threshold}};
  return evaluate(data, metric, output_type, opts);
}

std::vector<neural_net::image_annotation>
object_detector::convert_yolo_to_annotations(
    const neural_net::float_array& yolo_map,
    const std::vector<std::pair<float, float>>& anchor_boxes,
    float min_confidence) {
  return turi::object_detection::convert_yolo_to_annotations(
      yolo_map, anchor_boxes, min_confidence);
}

std::shared_ptr<MLModelWrapper> object_detector::export_to_coreml(
    std::string filename, std::string short_desc,
    std::map<std::string, flexible_type> additional_user_defined,
    std::map<std::string, flexible_type> opts)
{
  // If called during training, synchronize the model first.
  const Checkpoint& checkpoint = read_checkpoint();

  size_t grid_height = read_state<size_t>("grid_height");
  size_t grid_width = read_state<size_t>("grid_width");

  std::string input_str = read_state<std::string>("feature");
  std::string coordinates_str = "coordinates";
  std::string confidence_str = "confidence";

  // No options provided defaults to include Non Maximum Suppression.
  if (opts.find("include_non_maximum_suppression") == opts.end()) {
    opts["include_non_maximum_suppression"] = 1;
  }

  if (opts["include_non_maximum_suppression"].to<bool>()){
    coordinates_str = "raw_coordinates";
    confidence_str = "raw_confidence";
    //Set default values if thresholds not provided.
    if (opts.find("iou_threshold") == opts.end()) {
      opts["iou_threshold"] = DEFAULT_NON_MAXIMUM_SUPPRESSION_THRESHOLD;
    }
    if (opts.find("confidence_threshold") == opts.end()) {
      opts["confidence_threshold"] = DEFAULT_CONFIDENCE_THRESHOLD_PREDICT;
    }
  }

  // Compute the string representation of the list of class labels.
  flex_string class_labels_str;
  flex_list class_labels = read_state<flex_list>("classes");
  if (!class_labels.empty()) {
    class_labels_str = class_labels.front().get<flex_string>();
  }
  for (size_t i = 1; i < class_labels.size(); ++i) {
    class_labels_str += "," + class_labels[i].get<flex_string>();
  }

  // Generate "user-defined" metadata.
  flex_dict user_defined_metadata = {
      {"model", read_state<flex_string>("model")},
      {"max_iterations", read_state<flex_int>("max_iterations")},
      {"training_iterations", read_state<flex_int>("training_iterations")},
      {"include_non_maximum_suppression", "False"},
      {"feature", read_state<flex_string>("feature")},
      {"annotations", read_state<flex_string>("annotations")},
      {"classes", class_labels_str},
      {"type", "object_detector"},
    };

  for(const auto& kvp : additional_user_defined) {
       user_defined_metadata.emplace_back(kvp.first, kvp.second);
  }

  if (opts["include_non_maximum_suppression"].to<bool>()){
    user_defined_metadata.emplace_back("include_non_maximum_suppression", "True");
    user_defined_metadata.emplace_back("confidence_threshold", opts["confidence_threshold"]);
    user_defined_metadata.emplace_back("iou_threshold", opts["iou_threshold"]);
  }

  user_defined_metadata.emplace_back("version", opts["version"]);

  neural_net::pipeline_spec spec =
      checkpoint.ExportToCoreML(input_str, coordinates_str, confidence_str);

  std::shared_ptr<MLModelWrapper> model_wrapper = export_object_detector_model(
      std::move(spec), class_labels.size(),
      grid_height * grid_width * anchor_boxes().size(), std::move(class_labels),
      std::move(opts));

  model_wrapper->add_metadata({
      {"user_defined", std::move(user_defined_metadata)},
      {"short_description", short_desc}
  });

  if (!filename.empty()) {
    model_wrapper->save(filename);
  }

  return model_wrapper;
}

std::unique_ptr<data_iterator> object_detector::create_iterator(
    gl_sframe data, std::vector<std::string> class_labels, bool repeat,
    bool is_training) const {
  data_iterator::parameters iterator_params;

  // Check if data has annotations column
  std::string annotations_column_name = read_state<flex_string>("annotations");
  if (data.contains_column(annotations_column_name)) {
    iterator_params.annotations_column_name = annotations_column_name;
  }

  iterator_params.data = std::move(data);
  iterator_params.image_column_name = read_state<flex_string>("feature");
  iterator_params.class_labels = std::move(class_labels);
  iterator_params.repeat = repeat;
  iterator_params.is_training = is_training;

  std::string annotation_origin = read_state<flex_string>("annotation_origin");
  std::string annotation_scale = read_state<flex_string>("annotation_scale");
  std::string annotation_position = read_state<flex_string>("annotation_position");

  // Setting input for Image Origin
  if (annotation_origin == "top_left") {
      iterator_params.annotation_origin = data_iterator::annotation_origin_enum::TOP_LEFT;
  }
  else if (annotation_origin == "bottom_left") {
      iterator_params.annotation_origin = data_iterator::annotation_origin_enum::BOTTOM_LEFT;
  }

  // Setting input for Annotation Scale
  if (annotation_scale == "pixel") {
      iterator_params.annotation_scale = data_iterator::annotation_scale_enum::PIXEL;
  }
  else if (annotation_scale == "normalized") {
      iterator_params.annotation_scale = data_iterator::annotation_scale_enum::NORMALIZED;
  }

  // Setting input for Annotation Position
  if (annotation_position == "center") {
      iterator_params.annotation_position = data_iterator::annotation_position_enum::CENTER;
  }
  else if (annotation_position == "top_left") {
      iterator_params.annotation_position = data_iterator::annotation_position_enum::TOP_LEFT;
  }
  else if (annotation_position == "bottom_left") {
      iterator_params.annotation_position = data_iterator::annotation_position_enum::BOTTOM_LEFT;
  }

  return create_iterator(iterator_params);
}

std::unique_ptr<data_iterator> object_detector::create_iterator(
    data_iterator::parameters iterator_params) const
{
    return std::unique_ptr<data_iterator>(
        new simple_data_iterator(iterator_params));
}

std::unique_ptr<compute_context> object_detector::create_compute_context() const
{
  return compute_context::create();
}

void object_detector::init_training(gl_sframe data,
                                    std::string annotations_column_name,
                                    std::string image_column_name,
                                    variant_type validation_data,
                                    std::map<std::string, flexible_type> opts) {
  // Extract 'mlmodel_path' from the options, to avoid storing it as a model
  // field.
  auto mlmodel_path_iter = opts.find("mlmodel_path");
  if (mlmodel_path_iter == opts.end()) {
    log_and_throw("Expected option \"mlmodel_path\" not found.");
  }
  const std::string mlmodel_path = mlmodel_path_iter->second;
  opts.erase(mlmodel_path_iter);

  // Read options from user.
  init_options(opts);

  // Choose a random seed if not set.
  if (read_state<flexible_type>("random_seed") == FLEX_UNDEFINED) {
    std::random_device random_device;
    int random_seed = static_cast<int>(random_device());
    add_or_update_state({{"random_seed", random_seed}});
  }

  // Record the relevant column names upfront, for use in create_iterator. Also
  // values fixed by this version of the toolkit.
  add_or_update_state({{"annotations", annotations_column_name},
                       {"feature", image_column_name},
                       {"model", read_state<std::string>("model")}});

  // Perform random validation split if necessary.
  std::tie(training_data_, validation_data_) =
      supervised::create_validation_data(data, validation_data,
                                         read_state<int>("random_seed"));

  // Bind the data to a data iterator.
  std::vector<std::string> class_labels =
      read_state<std::vector<std::string>>("classes");
  std::unique_ptr<data_iterator> iterator =
      create_iterator(training_data_, /* expected class_labels */ class_labels,
                      /* repeat */ true, /* is_training */ true);

  // Instantiate the compute context.
  std::unique_ptr<compute_context> context = create_compute_context();
  if (context == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  // Infer values for unspecified options. Note that this depends on training
  // data statistics and the compute context, initialized above.
  // TODO: Move this into DarknetYOLOModelTrainer, since these heuristics are
  // model-specific.
  infer_derived_options(context.get(), iterator.get());

  // Set additional model fields.
  flex_int grid_height = read_state<flex_int>("grid_height");
  flex_int grid_width = read_state<flex_int>("grid_width");
  std::array<flex_int, 3> input_image_shape =  // Using CoreML CHW format.
      {{3, grid_height * SPATIAL_REDUCTION, grid_width * SPATIAL_REDUCTION}};
  const std::vector<std::string>& classes = iterator->class_labels();
  add_or_update_state({
      {"classes", flex_list(classes.begin(), classes.end())},
      {"input_image_shape",
       flex_list(input_image_shape.begin(), input_image_shape.end())},
      {"num_bounding_boxes", iterator->num_instances()},
      {"num_classes", iterator->class_labels().size()},
      {"num_examples", training_data_.size()},
      {"training_epochs", 0},
      {"training_iterations", 0},
  });
  // TODO: The original Python implementation also exposed "anchors",
  // "non_maximum_suppression_threshold", and "training_time".

  int batch_size = read_state<int>("batch_size");
  Config config;
  config.max_iterations = static_cast<int>(get_max_iterations());
  config.batch_size = batch_size;
  config.output_height = static_cast<int>(grid_height);
  config.output_width = static_cast<int>(grid_width);
  config.num_classes = static_cast<int>(get_num_classes());
  config.random_seed = read_state<int>("random_seed");

  // Load the pre-trained model from the provided path. The final layers are
  // initialized randomly using the random seed above, using the number of
  // classes observed by the training_data_iterator_ above.
  std::unique_ptr<ModelTrainer> trainer =
      create_trainer(config, mlmodel_path, std::move(context));

  // Establish training pipeline.
  connect_trainer(std::move(trainer), std::move(iterator), batch_size);
}

std::unique_ptr<ModelTrainer> object_detector::create_trainer(
    const Config& config, const std::string& pretrained_model_path,
    std::unique_ptr<neural_net::compute_context> context) const {
  // For now, we only support darknet-yolo. Load the pre-trained model and
  // randomly initialize the final layers.
  checkpoint_.reset(new DarknetYOLOCheckpoint(config, pretrained_model_path));
  return checkpoint_->CreateModelTrainer(context.get());
}

void object_detector::resume_training(gl_sframe data,
                                      variant_type validation_data) {
  // Perform random validation split if necessary.
  std::tie(training_data_, validation_data_) =
      supervised::create_validation_data(data, validation_data,
                                         read_state<int>("random_seed"));

  // Bind the data to a data iterator.
  flex_list class_labels = read_state<flex_list>("classes");
  std::unique_ptr<data_iterator> iterator = create_iterator(
      training_data_,
      std::vector<std::string>(class_labels.begin(), class_labels.end()),
      /* repeat */ true, /* is_training */ true);

  // Instantiate the compute context.
  std::unique_ptr<compute_context> context = create_compute_context();
  if (context == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  // Load the model from the current checkpoint.
  std::unique_ptr<ModelTrainer> trainer =
      checkpoint_->CreateModelTrainer(context.get());

  // Establish training pipeline.
  connect_trainer(std::move(trainer), std::move(iterator),
                  read_state<int>("batch_size"));
}

void object_detector::connect_trainer(std::unique_ptr<ModelTrainer> trainer,
                                      std::unique_ptr<data_iterator> iterator,
                                      int batch_size) {
  // Subscribe to the trainer using futures, for compatibility with our
  // current synchronous API surface.
  int offset = read_state<int>("training_iterations");
  training_futures_ =
      trainer->AsTrainingBatchPublisher(std::move(iterator), batch_size, offset)
          ->AsFutures();
  checkpoint_futures_ = trainer->AsCheckpointPublisher()->AsFutures();

  // Begin printing progress, after any logging triggered above.
  if (read_state<bool>("verbose")) {
    training_table_printer_.reset(new table_printer(
        {{"Iteration", 12}, {"Loss", 12}, {"Elapsed Time", 12}}));
    training_table_printer_->print_header();
  }
}

void object_detector::iterate_training() {
  // Training must have been initialized.
  ASSERT_TRUE(training_futures_ != nullptr);

  // If we have a local checkpoint, it will no longer be valid.
  checkpoint_.reset();

  // We want to have no more than two pending batches at a time (double
  // buffering). We're about to add a new one, so wait until we only have one.
  wait_for_training_batches(1);

  // Update the model fields tracking how much training we've done.
  flex_int iteration_idx = get_training_iterations();
  flex_int batch_size = read_state<flex_int>("batch_size");
  flex_int num_examples = read_state<flex_int>("num_examples");
  add_or_update_state({
      { "training_iterations", iteration_idx + 1 },
      { "training_epochs", (iteration_idx + 1) * batch_size / num_examples },
  });

  // Trigger another training batch.
  std::future<std::unique_ptr<TrainingOutputBatch>> training_batch =
      training_futures_->Next();

  // Save the result, which is a future that can synchronize with the
  // completion of this batch.
  pending_training_batches_.emplace(std::move(training_batch));
}

void object_detector::synchronize_training() { wait_for_training_batches(); }

float_array_map object_detector::strip_fwd(
    const float_array_map& raw_model_params) const {
  // Strip the substring "_fwd" from any parameter names, for compatibility with
  // the compute backend.
  // TODO: Someday, this will all be an implementation detail of each
  // model_backend implementation, once they actually take model_spec values as
  // inputs. Or maybe we should just not use "_fwd" in the exported model?
  // TODO: Remove this model-specific code once the inference path no longer
  // needs it.
  float_array_map model_params;
  for (const float_array_map::value_type& kv : raw_model_params) {
    const std::string modifier = "_fwd";
    std::string name = kv.first;
    size_t pos = name.find(modifier);
    if (pos != std::string::npos) {
      name.erase(pos, modifier.size());
    }
    model_params[name] = kv.second;
  }

  return model_params;
}

flex_int object_detector::get_max_iterations() const {
  return read_state<flex_int>("max_iterations");
}

flex_int object_detector::get_training_iterations() const {
  return read_state<flex_int>("training_iterations");
}

flex_int object_detector::get_num_classes() const {
  return read_state<flex_int>("num_classes");
}

void object_detector::wait_for_training_batches(size_t max_pending) {
  // TODO: Once we adopt an asynchronous API, we can let this "double buffering"
  // fall out of the back pressure we apply to the Combine pipeline.

  while (pending_training_batches_.size() > max_pending) {

    // Pop the first pending batch from the queue.
    TrainingOutputBatch training_batch =
        *pending_training_batches_.front().get();
    pending_training_batches_.pop();
    int iteration_id = training_batch.iteration_id;
    const shared_float_array& loss_batch = training_batch.loss;

    // TODO: Move this into object_detection::ModelTrainer once the
    // model_backend interface adopts an async API, so that this post-processing
    // doesn't prematurely trigger a wait on a future.

    // Compute the loss for this batch.
    float batch_loss = std::accumulate(
        loss_batch.data(), loss_batch.data() + loss_batch.size(), 0.f,
        [](float a, float b) { return a + b; });

    // Update our rolling average (smoothed) loss.
    auto loss_it = state.find("training_loss");
    if (loss_it == state.end()) {
      loss_it = state.emplace("training_loss", variant_type(batch_loss)).first;
    } else {
      float smoothed_loss = variant_get_value<flex_float>(loss_it->second);
      smoothed_loss = 0.9f * smoothed_loss + 0.1f * batch_loss;
      loss_it->second = smoothed_loss;
    }

    // Report progress if we have an active table printer.
    if (training_table_printer_) {
      flex_float loss = variant_get_value<flex_float>(loss_it->second);
      training_table_printer_->print_progress_row(iteration_id, iteration_id,
                                                  loss, progress_time());
    }
  }
}

void object_detector::update_model_metrics(gl_sframe data,
                                           gl_sframe validation_data) {
  std::map<std::string, variant_type> metrics;

  // Compute training metrics.
  variant_type training_metrics_raw =
      perform_evaluation(data, "all", "dict", DEFAULT_CONFIDENCE_THRESHOLD_EVALUATE,
        DEFAULT_NON_MAXIMUM_SUPPRESSION_THRESHOLD);
  variant_map_type training_metrics =
      variant_get_value<variant_map_type>(training_metrics_raw);
  for (const auto& kv : training_metrics) {
    metrics["training_" + kv.first] = kv.second;
  }

  // Compute validation metrics if necessary.
  if (!validation_data.empty()) {
    variant_type validation_metrics_raw =
        perform_evaluation(validation_data, "all", "dict", DEFAULT_CONFIDENCE_THRESHOLD_EVALUATE,
         DEFAULT_NON_MAXIMUM_SUPPRESSION_THRESHOLD);
    variant_map_type validation_metrics =
        variant_get_value<variant_map_type>(validation_metrics_raw);
    for (const auto& kv : validation_metrics) {
      metrics["validation_" + kv.first] = kv.second;
    }
  }

  // Add metrics to model state.
  add_or_update_state(metrics);
}

}  // object_detection
}  // turi
