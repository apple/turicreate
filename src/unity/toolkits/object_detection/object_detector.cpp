/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/object_detection/object_detector.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

#include <logger/assertions.hpp>
#include <logger/logger.hpp>
#include <random/random.hpp>
#include <unity/toolkits/coreml_export/neural_net_models_exporter.hpp>
#include <unity/toolkits/object_detection/od_evaluation.hpp>
#include <unity/toolkits/object_detection/od_yolo.hpp>

using turi::coreml::MLModelWrapper;
using turi::neural_net::compute_context;
using turi::neural_net::deferred_float_array;
using turi::neural_net::float_array_map;
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

// We assume RGB input.
constexpr int NUM_INPUT_CHANNELS = 3;

// Annotated and predicted bounding boxes are defined relative to a
// GRID_SIZE x GRID_SIZE grid laid over the image.
constexpr int GRID_SIZE = 13;

// The spatial reduction depends on the input size of the pre-trained model
// (relative to the grid size).
// TODO: When we support alternative base models, we will have to generalize.
constexpr int SPATIAL_REDUCTION = 32;

// For the MPS implementation of the darknet-yolo model, the loss must be scaled
// up to avoid underflow in the fp16 gradient images. The learning rate is
// correspondingly divided by the same multiple to make training mathematically
// equivalent. The update is done in fp32, which is why this trick works. The
// loss presented to the user is presented in the original scale.
// TODO: Only apply this for MPS, once this code also supports MXNet.
constexpr float MPS_LOSS_MULTIPLIER = 8;

constexpr float BASE_LEARNING_RATE = 0.001f / MPS_LOSS_MULTIPLIER;

constexpr float DEFAULT_NON_MAXIMUM_SUPPRESSION_THRESHOLD = 0.45f;

// Predictions with confidence scores below this threshold will be discarded
// before generation precision-recall curves.
constexpr float EVAL_CONFIDENCE_THRESHOLD = 0.001f;

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

// These are the fixed values that the Python implementation currently passes
// into TCMPS.
// TODO: These should be exposed in a way that facilitates experimentation.
// TODO: A struct instead of a map would be nice, too.
float_array_map get_training_config() {
  float_array_map config;
  config["gradient_clipping"]        =
      shared_float_array::wrap(0.025f * MPS_LOSS_MULTIPLIER);
  config["learning_rate"]            =
      shared_float_array::wrap(BASE_LEARNING_RATE);
  config["mode"]                     = shared_float_array::wrap(0.f);
  config["od_include_loss"]          = shared_float_array::wrap(1.0f);
  config["od_include_network"]       = shared_float_array::wrap(1.0f);
  config["od_max_iou_for_no_object"] = shared_float_array::wrap(0.3f);
  config["od_min_iou_for_object"]    = shared_float_array::wrap(0.7f);
  config["od_rescore"]               = shared_float_array::wrap(1.0f);
  config["od_scale_class"]           =
      shared_float_array::wrap(2.0f * MPS_LOSS_MULTIPLIER);
  config["od_scale_no_object"]       =
      shared_float_array::wrap(5.0f * MPS_LOSS_MULTIPLIER);
  config["od_scale_object"]          =
      shared_float_array::wrap(100.0f * MPS_LOSS_MULTIPLIER);
  config["od_scale_wh"]              =
      shared_float_array::wrap(10.0f * MPS_LOSS_MULTIPLIER);
  config["od_scale_xy"]              =
      shared_float_array::wrap(10.0f * MPS_LOSS_MULTIPLIER);
  config["use_sgd"]                  = shared_float_array::wrap(1.0f);
  config["weight_decay"]             = shared_float_array::wrap(0.0005f);
  return config;
}

float_array_map get_prediction_config() {
  float_array_map config;
  config["mode"]                     = shared_float_array::wrap(2.0f);
  config["od_include_loss"]          = shared_float_array::wrap(0.0f);
  config["od_include_network"]       = shared_float_array::wrap(1.0f);
  return config;
}

image_augmenter::options get_augmentation_options(flex_int batch_size) {
  image_augmenter::options opts;

  // Specify the fixed image size expected by the neural network.
  opts.batch_size = static_cast<size_t>(batch_size);
  opts.output_width = GRID_SIZE * SPATIAL_REDUCTION;
  opts.output_height = GRID_SIZE * SPATIAL_REDUCTION;

  // Apply random crops.
  opts.crop_prob = 0.9f;
  opts.crop_opts.min_aspect_ratio = 0.8f;
  opts.crop_opts.max_aspect_ratio = 1.25f;
  opts.crop_opts.min_area_fraction = 0.15f;
  opts.crop_opts.max_area_fraction = 1.f;
  opts.crop_opts.min_object_covered = 0.f;
  opts.crop_opts.max_attempts = 50;
  opts.crop_opts.min_eject_coverage = 0.5f;

  // Apply random padding.
  opts.pad_prob = 0.9f;
  opts.pad_opts.min_aspect_ratio = 0.8f;
  opts.pad_opts.max_aspect_ratio = 1.25f;
  opts.pad_opts.min_area_fraction = 1.f;
  opts.pad_opts.max_area_fraction = 2.f;
  opts.pad_opts.max_attempts = 50;

  // Allow mirror images.
  opts.horizontal_flip_prob = 0.5f;

  // Apply random perturbations to color.
  opts.brightness_max_jitter = 0.05f;
  opts.contrast_max_jitter = 0.05f;
  opts.saturation_max_jitter = 0.05f;
  opts.hue_max_jitter = 0.05f;

  return opts;
}

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

  // The default values for some options request automatic configuration from
  // the training data.
  ASSERT_TRUE(training_data_iterator_ != nullptr);

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

  // Validate user-provided options.
  options.set_options(opts);

  // Report to the user what GPU(s) is being used.
  std::vector<std::string> gpu_names = training_compute_context_->gpu_names();
  if (gpu_names.empty()) {
    logprogress_stream << "Using CPU to create model";
  } else {
    std::string gpu_names_string = gpu_names[0];
    for (size_t i = 1; i < gpu_names.size(); ++i) {
      gpu_names_string += ", " + gpu_names[i];
    }
    logprogress_stream << "Using "
                       << (gpu_names.size() > 1 ? "GPUs" : "GPU")
                       << " to create model ("
                       << gpu_names_string << ")";
  }

  // Configure the batch size automatically if not set.
  if (options.value("batch_size") == FLEX_UNDEFINED) {

    flex_int batch_size = DEFAULT_BATCH_SIZE;
    size_t memory_budget = training_compute_context_->memory_budget();
    if (memory_budget < MEMORY_REQUIRED_FOR_DEFAULT_BATCH_SIZE) {
      batch_size /= 2;
    }
    // TODO: What feedback can we give if the user requests a batch size that
    // doesn't fit?

    logprogress_stream << "Setting 'batch_size' to " << batch_size;

    options.set_option("batch_size", batch_size);
  }

  // Configure targeted number of iterations automatically if not set.
  if (options.value("max_iterations") == FLEX_UNDEFINED) {
    flex_int max_iterations = estimate_max_iterations(
        static_cast<flex_int>(training_data_iterator_->num_instances()),
        options.value("batch_size"));

    logprogress_stream << "Setting 'max_iterations' to " << max_iterations;

    options.set_option("max_iterations", max_iterations);
  }

  // Write model fields.
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

size_t object_detector::get_version() const {
  return OBJECT_DETECTOR_VERSION;
}

void object_detector::save_impl(oarchive& oarc) const {}

void object_detector::load_version(iarchive& iarc, size_t version) {}

void object_detector::train(gl_sframe data,
                            std::string annotations_column_name,
                            std::string image_column_name,
                            variant_type validation_data,
                            std::map<std::string, flexible_type> opts) {

  // Begin printing progress.
  // TODO: Make progress printing optional.
  training_table_printer_.reset(new table_printer(
      {{ "Iteration", 12}, {"Loss", 12}, {"Elapsed Time", 12}}));

  // Instantiate the training dependencies: data iterator, image augmenter,
  // backend NN model.
  init_train(std::move(data), std::move(annotations_column_name),
             std::move(image_column_name), std::move(opts));

  // Perform all the iterations at once.
  while (get_training_iterations() < get_max_iterations()) {
    perform_training_iteration();
  }

  // Wait for any outstanding batches to finish.
  wait_for_training_batches();

  // Finish printing progress.
  training_table_printer_->print_footer();
  training_table_printer_.reset();

  // Sync trained weights to our local storage of the NN weights.
  float_array_map raw_trained_weights = training_model_->export_weights();
  float_array_map trained_weights;
  for (const auto& kv : raw_trained_weights) {
    // Convert keys from the model_backend names (e.g. "conv7_weight") to the
    // names we're exporting to CoreML (e.g. "conv7_fwd_weight").
    const std::string modifier = "_fwd";
    std::string key = kv.first;
    std::string::iterator it = std::find(key.begin(), key.end(), '_');
    key.insert(it, modifier.begin(), modifier.end());
    trained_weights[key] = kv.second;
  }
  nn_spec_->update_params(trained_weights);

  // Compute training and validation metrics.
  gl_sframe val_data;
  if (variant_is<gl_sframe>(validation_data)) {
    val_data = variant_get_value<gl_sframe>(validation_data);
  }
  update_model_metrics(data, val_data);
}

variant_map_type object_detector::evaluate(
    gl_sframe data, std::string metric,
    std::map<std::string, flexible_type> opts) {
  return perform_evaluation(std::move(data), std::move(metric));
}

// TODO: Should accept model_backend as an optional argument to avoid
// instantiating a new backend during training. Or just check to see if an
// existing backend is available?
variant_map_type object_detector::perform_evaluation(gl_sframe data,
                                                     std::string metric) {
  std::vector<std::string> metrics;
  static constexpr char AP[] = "average_precision";
  static constexpr char MAP[] = "mean_average_precision";
  static constexpr char AP50[] = "average_precision_50";
  static constexpr char MAP50[] = "mean_average_precision_50";
  std::vector<std::string> all_metrics = {AP,MAP,AP50,MAP50};
  if (std::find(all_metrics.begin(), all_metrics.end(), metric) != all_metrics.end()) {
    metrics = {metric};
  }
  else if (metric == "auto") {
    metrics = {AP50,MAP50};
  }
  else if (metric == "all" || metric == "report") {
    metrics = all_metrics;
  }
  else {
    log_and_throw("Metric " + metric + " not supported");
  }
  
  
  std::string image_column_name = read_state<flex_string>("feature");
  std::string annotations_column_name = read_state<flex_string>("annotations");
  flex_list class_labels = read_state<flex_list>("classes");
  size_t batch_size = static_cast<size_t>(options.value("batch_size"));
  float iou_threshold =
      read_state<flex_float>("non_maximum_suppression_threshold");

  // Bind the data to a data iterator.
  std::unique_ptr<data_iterator> data_iter = create_iterator(
      data, std::vector<std::string>(class_labels.begin(), class_labels.end()),
      /* repeat */ false);

  // Instantiate the compute context.
  std::unique_ptr<compute_context> ctx = create_compute_context();
  if (ctx == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  // Instantiate the data augmenter. Don't enable any of the actual
  // augmentations, just resize the input images to the desired shape.
  image_augmenter::options augmenter_opts;
  augmenter_opts.batch_size = batch_size;
  augmenter_opts.output_width = GRID_SIZE * SPATIAL_REDUCTION;
  augmenter_opts.output_height = GRID_SIZE * SPATIAL_REDUCTION;
  std::unique_ptr<image_augmenter> augmenter =
      ctx->create_image_augmenter(augmenter_opts);

  // Instantiate the NN backend.
  // For each anchor box, we have 4 bbox coords + 1 conf + one-hot class labels
  int num_outputs_per_anchor = 5 + static_cast<int>(class_labels.size());
  int num_output_channels = num_outputs_per_anchor * anchor_boxes().size();
  std::unique_ptr<model_backend> model = ctx->create_object_detector(
      /* n */     options.value("batch_size"),
      /* c_in */  NUM_INPUT_CHANNELS,
      /* h_in */  GRID_SIZE * SPATIAL_REDUCTION,
      /* w_in */  GRID_SIZE * SPATIAL_REDUCTION,
      /* c_out */ num_output_channels,
      /* h_out */ GRID_SIZE,
      /* w_out */ GRID_SIZE,
      get_prediction_config(),
      get_model_params());

  // Initialize the metric calculator
  average_precision_calculator calculator(class_labels);

  // To support double buffering, use a queue of pending inference results.
  std::queue<image_augmenter::result> pending_batches;

  // Helper function to process results until the queue reaches a given size.
  auto pop_until_size = [&](size_t remaining) {

    while (pending_batches.size() > remaining) {

      // Pop one batch from the queue.
      image_augmenter::result batch = pending_batches.front();
      pending_batches.pop();

      for (size_t i = 0; i < batch.annotations_batch.size(); ++i) {

        // For this row (corresponding to one image), extract the prediction.
        shared_float_array raw_prediction = batch.image_batch[i];

        // Translate the raw output into predicted labels and bounding boxes.
        std::vector<image_annotation> predicted_annotations =
            convert_yolo_to_annotations(raw_prediction, anchor_boxes(),
                                        EVAL_CONFIDENCE_THRESHOLD);

        // Remove overlapping predictions.
        predicted_annotations = apply_non_maximum_suppression(
            std::move(predicted_annotations), iou_threshold);

        // Tally the predictions and ground truth labels.
        calculator.add_row(predicted_annotations, batch.annotations_batch[i]);
      }
    }
  };

  // Iterate through the data once.
  std::vector<labeled_image> input_batch = data_iter->next_batch(batch_size);
  while (!input_batch.empty()) {

    // Wait until we have just one asynchronous batch outstanding. The work
    // below should be concurrent with the neural net inference for that batch.
    pop_until_size(1);

    image_augmenter::result result_batch;

    // Instead of giving the ground truth data to the image augmenter and the
    // neural net, instead save them for later, pairing them with the future
    // predictions.
    result_batch.annotations_batch.resize(input_batch.size());
    for (size_t i = 0; i < input_batch.size(); ++i) {
      result_batch.annotations_batch[i] = std::move(input_batch[i].annotations);
      input_batch[i].annotations.clear();
    }

    // Use the image augmenter to format the images into float arrays, and
    // submit them to the neural net.
    image_augmenter::result prepared_input_batch =
        augmenter->prepare_images(std::move(input_batch));
    std::map<std::string, shared_float_array> prediction_results =
        model->predict({{"input", prepared_input_batch.image_batch}});
    result_batch.image_batch = prediction_results.at("output");

    // Add the pending result to our queue and move on to the next input batch.
    pending_batches.push(std::move(result_batch));
    input_batch = data_iter->next_batch(batch_size);
  }

  // Process all remaining batches.
  pop_until_size(0);

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

  return result_map;
}

std::unique_ptr<model_spec> object_detector::init_model(
    const std::string& pretrained_mlmodel_path) const {

  // All of this presumes that the pre-trained model is the darknet model from
  // our first object detector implementation....

  // TODO: Make this more generalizable.

  // Start with parameters from the pre-trained model.
  std::unique_ptr<model_spec> nn_spec(new model_spec(pretrained_mlmodel_path));

  // Verify that the pre-trained model ends with the expected leakyrelu6 layer.
  // TODO: Also verify that activation shape here is [1024, 13, 13]?
  if (!nn_spec->has_layer_output("leakyrelu6_fwd")) {
    log_and_throw("Expected leakyrelu6_fwd layer in NeuralNetwork parsed from "
                  + pretrained_mlmodel_path);
  }

  // Append conv7, initialized using the Xavier method (with base magnitude 3).
  // The conv7 weights have shape [1024, 1024, 3, 3], so fan in and fan out are
  // both 1024*3*3.
  xavier_weight_initializer conv7_init_fn(1024*3*3, 1024*3*3);
  nn_spec->add_convolution(/* name */                "conv7_fwd",
                           /* input */               "leakyrelu6_fwd",
                           /* num_output_channels */ 1024,
                           /* num_kernel_channels */ 1024,
                           /* kernel_height */       3,
                           /* kernel_width */        3,
                           /* stride_height */       1,
                           /* stride_width */        1,
                           /* padding */             padding_type::SAME,
                           /* weight_init_fn */      conv7_init_fn);

  // Append batchnorm7.
  nn_spec->add_batchnorm(/* name */                  "batchnorm7_fwd",
                         /* input */                 "conv7_fwd",
                         /* num_channels */          1024,
                         /* epsilon */               0.00001f);

  // Append leakyrelu7.
  nn_spec->add_leakyrelu(/* name */                  "leakyrelu7_fwd",
                         /* input */                 "batchnorm7_fwd",
                         /* alpha */                 0.1f);

  // Append conv8.
  static constexpr float CONV8_MAGNITUDE = 0.00005f;
  const size_t num_classes = training_data_iterator_->class_labels().size();
  const size_t num_predictions = 5 + num_classes;  // Per anchor box
  const size_t conv8_c_out = anchor_boxes().size() * num_predictions;
  auto conv8_weight_init_fn = [](float* w, float* w_end) {
    while (w != w_end) {
      *w++ = random::fast_uniform(-CONV8_MAGNITUDE, CONV8_MAGNITUDE);
    }
  };
  auto conv8_bias_init_fn = [num_predictions](float* w, float* w_end) {
    while (w < w_end) {
      // Initialize object confidence low, preventing an unnecessary adjustment
      // period toward conservative estimates
      w[4] = -6.f;

      // Iterate through each anchor box.
      w += num_predictions;
    }
  };
  nn_spec->add_convolution(/* name */                "conv8_fwd",
                           /* input */               "leakyrelu7_fwd",
                           /* num_output_channels */ conv8_c_out,
                           /* num_kernel_channels */ 1024,
                           /* kernel_height */       1,
                           /* kernel_width */        1,
                           /* stride_height */       1,
                           /* stride_width */        1,
                           /* padding */             padding_type::SAME,
                           /* weight_init_fn */      conv8_weight_init_fn,
                           /* bias_init_fn */        conv8_bias_init_fn);

  return nn_spec;
}

std::shared_ptr<MLModelWrapper> object_detector::export_to_coreml(
    std::string filename, std::map<std::string, flexible_type> opts) {

  // Initialize the result with the learned layers from the model_backend.
  model_spec yolo_nn_spec(nn_spec_->get_coreml_spec());
  
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
      opts["iou_threshold"] = 0.45;
    }
    if (opts.find("confidence_threshold") == opts.end()) {
      opts["confidence_threshold"] = 0.25;
    }
  }

  // Add the layers that convert to intelligible predictions.
  add_yolo(&yolo_nn_spec, coordinates_str, confidence_str, "conv8_fwd",
           anchor_boxes(), read_state<flex_int>("num_classes"),
           GRID_SIZE, GRID_SIZE);

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

  
  if (opts["include_non_maximum_suppression"].to<bool>()){
    user_defined_metadata.emplace_back("include_non_maximum_suppression", "True");
    user_defined_metadata.emplace_back("confidence_threshold", opts["confidence_threshold"]);
    user_defined_metadata.emplace_back("iou_threshold", opts["iou_threshold"]);
  }

  // TODO: Should we also be adding the non-user-defined keys, such as
  // "version" and "shortDescription", or is that up to the frontend?

  std::shared_ptr<MLModelWrapper> model_wrapper =
      export_object_detector_model(yolo_nn_spec,
                                   GRID_SIZE * SPATIAL_REDUCTION,
                                   GRID_SIZE * SPATIAL_REDUCTION,
                                   class_labels.size(),
                                   GRID_SIZE*GRID_SIZE * anchor_boxes().size(),
                                   std::move(user_defined_metadata), std::move(class_labels),
                                   std::move(opts));

  if (!filename.empty()) {
    model_wrapper->save(filename);
  }

  return model_wrapper;
}

std::unique_ptr<data_iterator> object_detector::create_iterator(
    gl_sframe data, std::vector<std::string> class_labels, bool repeat) const
{
  data_iterator::parameters iterator_params;
  iterator_params.data = std::move(data);
  iterator_params.annotations_column_name =
      read_state<flex_string>("annotations");
  iterator_params.image_column_name = read_state<flex_string>("feature");
  iterator_params.class_labels = std::move(class_labels);
  iterator_params.repeat = repeat;

  return std::unique_ptr<data_iterator>(
      new simple_data_iterator(iterator_params));  
}

std::unique_ptr<compute_context> object_detector::create_compute_context() const
{
  return compute_context::create();
}

void object_detector::init_train(gl_sframe data,
                                 std::string annotations_column_name,
                                 std::string image_column_name,
                                 std::map<std::string, flexible_type> opts) {
  // Record the relevant column names upfront, for use in create_iterator. Also
  // values fixed by this version of the toolkit.
  std::array<flex_int, 3> input_image_shape =  // Using CoreML CHW format.
      {{3, GRID_SIZE * SPATIAL_REDUCTION, GRID_SIZE * SPATIAL_REDUCTION}};
  add_or_update_state({
      { "annotations", annotations_column_name },
      { "feature", image_column_name },
      { "input_image_shape", flex_list(input_image_shape.begin(),
                                       input_image_shape.end()) },
      { "model", "darknet-yolo" },
      { "non_maximum_suppression_threshold",
        DEFAULT_NON_MAXIMUM_SUPPRESSION_THRESHOLD },
  });

  // Bind the data to a data iterator.
  training_data_iterator_ = create_iterator(
      data, /* expected class_labels */ {}, /* repeat */ true);

  // Instantiate the compute context.
  training_compute_context_ = create_compute_context();
  if (training_compute_context_ == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  // Extract 'mlmodel_path' from the options, to avoid storing it as a model
  // field.
  auto mlmodel_path_iter = opts.find("mlmodel_path");
  if (mlmodel_path_iter == opts.end()) {
    log_and_throw("Expected option \"mlmodel_path\" not found.");
  }
  const std::string mlmodel_path = mlmodel_path_iter->second;
  opts.erase(mlmodel_path_iter);

  // Load the pre-trained model from the provided path.
  nn_spec_ = init_model(mlmodel_path);

  // Validate options, and infer values for unspecified options. Note that this
  // depends on training data statistics in the general case.
  init_options(opts);

  // Set additional model fields.
  const std::vector<std::string>& classes =
      training_data_iterator_->class_labels();
  add_or_update_state({
      { "classes", flex_list(classes.begin(), classes.end()) },
      { "num_bounding_boxes", training_data_iterator_->num_instances() },
      { "num_classes", training_data_iterator_->class_labels().size() },
      { "num_examples", data.size() },
      { "training_epochs", 0 },
      { "training_iterations", 0 },
  });
  // TODO: The original Python implementation also exposed "anchors",
  // "non_maximum_suppression_threshold", and "training_time".

  // Instantiate the data augmenter.
  training_data_augmenter_ = training_compute_context_->create_image_augmenter(
      get_augmentation_options(options.value("batch_size")));

  // Instantiate the NN backend.
  int num_outputs_per_anchor =  // 4 bbox coords + 1 conf + one-hot class labels
      5 + static_cast<int>(training_data_iterator_->class_labels().size());
  int num_output_channels = num_outputs_per_anchor * anchor_boxes().size();
  training_model_ = training_compute_context_->create_object_detector(
      /* n */     options.value("batch_size"),
      /* c_in */  NUM_INPUT_CHANNELS,
      /* h_in */  GRID_SIZE * SPATIAL_REDUCTION,
      /* w_in */  GRID_SIZE * SPATIAL_REDUCTION,
      /* c_out */ num_output_channels,
      /* h_out */ GRID_SIZE,
      /* w_out */ GRID_SIZE,
      get_training_config(),
      get_model_params());

  // Print the header last, after any logging triggered by initialization above.
  if (training_table_printer_) {
    training_table_printer_->print_header();
  }
}

void object_detector::perform_training_iteration() {
  // Training must have been initialized.
  ASSERT_TRUE(training_data_iterator_ != nullptr);
  ASSERT_TRUE(training_data_augmenter_ != nullptr);
  ASSERT_TRUE(training_model_ != nullptr);

  // We want to have no more than two pending batches at a time (double
  // buffering). We're about to add a new one, so wait until we only have one.
  wait_for_training_batches(1);

  // Update iteration count and check learning rate schedule.
  // TODO: Abstract out the learning rate schedule.
  flex_int iteration_idx = get_training_iterations();
  flex_int max_iterations = get_max_iterations();
  if (iteration_idx == max_iterations / 2) {

    training_model_->set_learning_rate(BASE_LEARNING_RATE / 10.f);

  } else if (iteration_idx == max_iterations * 3 / 4) {

    training_model_->set_learning_rate(BASE_LEARNING_RATE / 100.f);

  } else if (iteration_idx == max_iterations) {

    // Handle any manually triggered iterations after the last planned one.
    training_model_->set_learning_rate(BASE_LEARNING_RATE / 1000.f);
  }

  // Update the model fields tracking how much training we've done.
  flex_int batch_size = read_state<flex_int>("batch_size");
  flex_int num_examples = read_state<flex_int>("num_examples");
  add_or_update_state({
      { "training_iterations", iteration_idx + 1 },
      { "training_epochs", (iteration_idx + 1) * batch_size / num_examples },
  });

  // Fetch the next batch of raw images and annotations.
  std::vector<labeled_image> image_batch =
      training_data_iterator_->next_batch(static_cast<size_t>(batch_size));

  // Perform data augmentation.
  image_augmenter::result augmenter_result =
      training_data_augmenter_->prepare_images(std::move(image_batch));

  // Encode the labels.
  shared_float_array label_batch =
      prepare_label_batch(augmenter_result.annotations_batch);

  // Submit the batch to the neural net model.
  std::map<std::string, shared_float_array> results = training_model_->train(
      { { "input",  augmenter_result.image_batch },
        { "labels", label_batch                  }  });
  shared_float_array loss_batch = results.at("loss");

  // Save the result, which is a future that can synchronize with the
  // completion of this batch.
  pending_training_batches_.emplace(iteration_idx, std::move(loss_batch));
}

float_array_map object_detector::get_model_params() const {

  float_array_map raw_model_params = nn_spec_->export_params_view();

  // Strip the substring "_fwd" from any parameter names, for compatibility with
  // the compute backend. (We preserve the substring in nn_spec_ for inclusion
  // in the final exported model.)
  // TODO: Someday, this will all be an implementation detail of each
  // model_backend implementation, once they actually take model_spec values as
  // inputs. Or maybe we should just not use "_fwd" in the exported model?
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

shared_float_array object_detector::prepare_label_batch(
    std::vector<std::vector<image_annotation>> annotations_batch) const {

  // Allocate a float buffer of sufficient size.
  size_t batch_size = static_cast<size_t>(options.value("batch_size"));
  size_t num_classes = training_data_iterator_->class_labels().size();
  size_t num_channels = anchor_boxes().size() * (5 + num_classes);  // C
  size_t batch_stride = GRID_SIZE * GRID_SIZE * num_channels;  // H * W * C
  std::vector<float> result(batch_size * batch_stride);  // NHWC

  // Write the structured annotations into the float buffer.
  float* result_out = result.data();
  if (annotations_batch.size() > batch_size) {
    annotations_batch.resize(batch_size);
  }
  for (const std::vector<image_annotation>& annotations : annotations_batch) {

    convert_annotations_to_yolo(annotations, GRID_SIZE, GRID_SIZE,
                                anchor_boxes().size(), num_classes, result_out);

    result_out += batch_stride;
  }

  // Wrap the resulting buffer and return it.
  return shared_float_array::wrap(
      std::move(result),
      { annotations_batch.size(), GRID_SIZE, GRID_SIZE, num_channels });
}

flex_int object_detector::get_max_iterations() const {
  return read_state<flex_int>("max_iterations");
}

flex_int object_detector::get_training_iterations() const {
  return read_state<flex_int>("training_iterations");
}

void object_detector::wait_for_training_batches(size_t max_pending) {

  while (pending_training_batches_.size() > max_pending) {

    // Pop the first pending batch from the queue.
    auto batch_it = pending_training_batches_.begin();
    size_t iteration_idx = batch_it->first;
    deferred_float_array loss_batch = std::move(batch_it->second);
    pending_training_batches_.erase(batch_it);

    // Compute the loss for this batch.
    float batch_loss = std::accumulate(
        loss_batch.data(), loss_batch.data() + loss_batch.size(), 0.f,
        [](float a, float b) { return a + b; });
    batch_loss /= MPS_LOSS_MULTIPLIER;

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
      training_table_printer_->print_progress_row(
          iteration_idx, iteration_idx + 1, loss, progress_time());
    }
  }
}

void object_detector::update_model_metrics(gl_sframe data,
                                           gl_sframe validation_data) {
  std::map<std::string, variant_type> metrics;

  // Compute training metrics.
  variant_map_type training_metrics = perform_evaluation(data, "all");
  for (const auto& kv : training_metrics) {
    metrics["training_" + kv.first] = kv.second;
  }

  // Compute validation metrics if necessary.
  if (!validation_data.empty()) {
    variant_map_type validation_metrics = perform_evaluation(validation_data,
                                                             "all");
    for (const auto& kv : validation_metrics) {
      metrics["validation_" + kv.first] = kv.second;
    }
  }

  // Add metrics to model state.
  add_or_update_state(metrics);
}

}  // object_detection
}  // turi 
