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
#include <numeric>

#include <logger/assertions.hpp>
#include <logger/logger.hpp>
#include <unity/toolkits/neural_net/coreml_import.hpp>

using turi::neural_net::cnn_module;
using turi::neural_net::deferred_float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::image_annotation;
using turi::neural_net::image_augmenter;
using turi::neural_net::labeled_image;
using turi::neural_net::load_network_params;
using turi::neural_net::shared_float_array;

namespace turi {
namespace object_detection {

namespace {

constexpr size_t OBJECT_DETECTOR_VERSION = 1;

constexpr int DEFAULT_BATCH_SIZE = 32;

// We assume RGB input.
constexpr int NUM_INPUT_CHANNELS = 3;

// Annotated and predicted bounding boxes are defined relative to a
// GRID_SIZE x GRID_SIZE grid laid over the image.
constexpr int GRID_SIZE = 13;

// Each bounding box is evaluated relative to a list of pre-defined sizes.
constexpr int NUM_ANCHOR_BOXES = 15;

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

  // Configure the batch size automatically if not set.
  if (options.value("batch_size") == FLEX_UNDEFINED) {
    // TODO: Reduce batch size when training on GPU with less than 4GB RAM.
    flex_int batch_size = DEFAULT_BATCH_SIZE;

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
                            std::map<std::string, flexible_type> options) {

  // Begin printing progress.
  // TODO: Make progress printing optional.
  training_table_printer_.reset(new table_printer(
      {{ "Iteration", 12}, {"Loss", 12}, {"Elapsed Time", 12}}));
  training_table_printer_->print_header();

  // Instantiate the training dependencies: data iterator, image augmenter,
  // backend NN module.
  init_train(std::move(data), std::move(annotations_column_name),
             std::move(image_column_name), std::move(options));

  // Perform all the iterations at once.
  while (get_training_iterations() < get_max_iterations()) {
    perform_training_iteration();
  }

  // Wait for any outstanding batches to finish.
  wait_for_training_batches();

  // Finish printing progress.
  training_table_printer_->print_footer();
  training_table_printer_.reset();
}

float_array_map object_detector::init_model_params(
    const std::string& pretrained_mlmodel_path) const {

  // All of this presumes that the pre-trained model is the darknet model from
  // our first object detector implementation....

  // TODO: Make this more generalizable.
  // TODO: Extract some of the pieces here into utility code.

  static constexpr size_t BUFFER_SIZE = 32;
  char name[BUFFER_SIZE];

  // Start with parameters from the pre-trained model.
  float_array_map model_params = load_network_params(pretrained_mlmodel_path);

  // Fill out the parameters for the batch-norm layers.
  std::array<size_t, 8> batchnorm_sizes =
      {16, 32, 64, 128, 256, 512, 1024, 1024};
  for (unsigned i = 0; i < batchnorm_sizes.size(); ++i) {
    size_t size = batchnorm_sizes[i];

    // The last layer does not come from the pre-trained model and must be
    // initialized from scratch.
    if (i == 7) {
      std::snprintf(name, BUFFER_SIZE, "batchnorm%u_beta", i);
      model_params[name] =
          shared_float_array::wrap(std::vector<float>(size, 0.f), {size});

      std::snprintf(name, BUFFER_SIZE, "batchnorm%u_gamma", i);
      model_params[name] =
          shared_float_array::wrap(std::vector<float>(size, 1.f), {size});
    }

    std::snprintf(name, BUFFER_SIZE, "batchnorm%u_running_mean", i);
    model_params[name] =
        shared_float_array::wrap(std::vector<float>(size, 0.f), {size});

    std::snprintf(name, BUFFER_SIZE, "batchnorm%u_running_var", i);
    model_params[name] =
        shared_float_array::wrap(std::vector<float>(size, 1.f), {size});
  }

  // Initialize conv7 using the Xavier method (with base magnitude 3).
  // The conv7 weights have shape [1024, 1024, 3, 3], so fan in and fan out are
  // both 1024*3*3.
  static constexpr size_t CONV7_FAN_IN = 1024 * 3 * 3;
  float conv7_magnitude = std::sqrt(3.f / CONV7_FAN_IN);
  std::vector<float> conv7_weights(1024*1024*3*3);
  for (float& w : conv7_weights) {
    w = random::fast_uniform(-conv7_magnitude, conv7_magnitude);
  }
  std::snprintf(name, BUFFER_SIZE, "conv%u_weight", 7);
  model_params[name] = shared_float_array::wrap(std::move(conv7_weights),
                                                { 1024, 1024, 3, 3});

  // Initialize conv8.

  static constexpr float CONV8_MAGNITUDE = 0.00005f;
  size_t num_classes = training_data_iterator_->class_labels().size();
  size_t num_predictions = 5 + num_classes;  // Per anchor box
  size_t c_out = NUM_ANCHOR_BOXES * num_predictions;
  std::vector<float> conv8_weights(c_out*1024*1*1);
  for (float& w : conv8_weights) {
    w = random::fast_uniform(-CONV8_MAGNITUDE, CONV8_MAGNITUDE);
  }
  std::snprintf(name, BUFFER_SIZE, "conv%u_weight", 8);
  model_params[name] = shared_float_array::wrap(std::move(conv8_weights),
                                                { c_out, 1024, 1, 1});

  // Initialize object confidence low, preventing an unnecessary adjustment
  // period toward conservative estimates
  std::vector<float> conv8_bias(c_out);  // Zero-initialized.
  for (size_t i = 0; i < NUM_ANCHOR_BOXES; ++i) {
    conv8_bias[i * num_predictions + 4] = -6.f;
  }
  std::snprintf(name, BUFFER_SIZE, "conv%u_bias", 8);
  model_params[name] = shared_float_array::wrap(std::move(conv8_bias),
                                                { c_out });

  return model_params;
}

std::unique_ptr<data_iterator> object_detector::create_iterator(
    gl_sframe data, std::string annotations_column_name,
    std::string image_column_name) const {

  data_iterator::parameters iterator_params;
  iterator_params.data = std::move(data);
  iterator_params.annotations_column_name = std::move(annotations_column_name);
  iterator_params.image_column_name = std::move(image_column_name);

  return std::unique_ptr<data_iterator>(
      new simple_data_iterator(iterator_params));  
}

std::unique_ptr<image_augmenter> object_detector::create_augmenter(
    const image_augmenter::options& opts) const {

  return image_augmenter::create(opts);
}

std::unique_ptr<cnn_module> object_detector::create_cnn_module (
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) const {

  return cnn_module::create_object_detector(n, c_in, h_in, w_in, c_out, h_out,
                                            w_out, config, weights);
}

void object_detector::init_train(gl_sframe data,
                                 std::string annotations_column_name,
                                 std::string image_column_name,
                                 std::map<std::string, flexible_type> opts) {

  // Bind the data to a data iterator.
  training_data_iterator_ = create_iterator(data, annotations_column_name,
                                            image_column_name);

  // Instantiate the data augmenter.
  image_augmenter::options aug_opts;
  aug_opts.output_width = GRID_SIZE * SPATIAL_REDUCTION;
  aug_opts.output_height = GRID_SIZE * SPATIAL_REDUCTION;
  training_data_augmenter_ = create_augmenter(aug_opts);

  // Load the pre-trained model from the provided path.
  auto mlmodel_path_iter = opts.find("mlmodel_path");
  if (mlmodel_path_iter == opts.end()) {
    log_and_throw("Expected option \"mlmodel_path\" not found.");
  }
  const std::string mlmodel_path = mlmodel_path_iter->second;
  float_array_map model_params = init_model_params(mlmodel_path);

  // Remove 'mlmodel_path' to avoid storing it as a model field.
  opts.erase(mlmodel_path_iter);

  // Validate options, and infer values for unspecified options. Note that this
  // depends on training data statistics in the general case.
  init_options(opts);

  // Set additional model fields.
  const std::vector<std::string>& classes =
      training_data_iterator_->class_labels();
  std::array<flex_int, 3> input_image_shape =  // Using CoreML CHW format.
      {3, GRID_SIZE * SPATIAL_REDUCTION, GRID_SIZE * SPATIAL_REDUCTION};
  add_or_update_state({
      { "annotations", annotations_column_name },
      { "classes", flex_list(classes.begin(), classes.end()) },
      { "feature", image_column_name },
      { "input_image_shape", flex_list(input_image_shape.begin(),
                                       input_image_shape.end()) },
      { "model", "darknet-yolo" },
      { "num_bounding_boxes", training_data_iterator_->num_instances() },
      { "num_classes", training_data_iterator_->class_labels().size() },
      { "num_examples", data.size() },
      { "training_epochs", 0 },
      { "training_iterations", 0 },
  });
  // TODO: The original Python implementation also exposed "anchors",
  // "non_maximum_suppression_threshold", and "training_time".

  // Instantiate the NN backend.
  int num_outputs_per_anchor =  // 4 bbox coords + 1 conf + one-hot class labels
      5 + static_cast<int>(training_data_iterator_->class_labels().size());
  int num_output_channels = num_outputs_per_anchor * NUM_ANCHOR_BOXES;
  training_module_ = create_cnn_module(
      /* n */     options.value("batch_size"),
      /* c_in */  NUM_INPUT_CHANNELS,
      /* h_in */  GRID_SIZE * SPATIAL_REDUCTION,
      /* w_in */  GRID_SIZE * SPATIAL_REDUCTION,
      /* c_out */ num_output_channels,
      /* h_out */ GRID_SIZE,
      /* w_out */ GRID_SIZE,
      get_training_config(),
      std::move(model_params));
}

void object_detector::perform_training_iteration() {
  // Training must have been initialized.
  ASSERT_TRUE(training_data_iterator_ != nullptr);
  ASSERT_TRUE(training_data_augmenter_ != nullptr);
  ASSERT_TRUE(training_module_ != nullptr);

  // We want to have no more than two pending batches at a time (double
  // buffering). We're about to add a new one, so wait until we only have one.
  wait_for_training_batches(1);

  // Update iteration count and check learning rate schedule.
  // TODO: Abstract out the learning rate schedule.
  flex_int iteration_idx = get_training_iterations();
  flex_int max_iterations = get_max_iterations();
  if (iteration_idx == max_iterations / 2) {

    training_module_->set_learning_rate(BASE_LEARNING_RATE / 10.f);

  } else if (iteration_idx == max_iterations * 3 / 4) {

    training_module_->set_learning_rate(BASE_LEARNING_RATE / 100.f);

  } else if (iteration_idx == max_iterations) {

    // Handle any manually triggered iterations after the last planned one.
    training_module_->set_learning_rate(BASE_LEARNING_RATE / 1000.f);
  }

  // Update the model fields tracking how much training we've done.
  flex_int batch_size =
      variant_get_value<flex_int>(get_value_from_state("batch_size"));
  flex_int num_examples =
      variant_get_value<flex_int>(get_value_from_state("num_examples"));
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

  // Submit the batch to the neural net module.
  deferred_float_array loss_batch = training_module_->train(
      augmenter_result.image_batch, label_batch);

  // Save the result, which is a future that can synchronize with the
  // completion of this batch.
  pending_training_batches_.emplace(iteration_idx, std::move(loss_batch));
}

shared_float_array object_detector::prepare_label_batch(
    std::vector<std::vector<image_annotation>> annotations_batch) const {

  // Allocate a float buffer of sufficient size.
  size_t num_classes = training_data_iterator_->class_labels().size();
  size_t num_channels = NUM_ANCHOR_BOXES * (5 + num_classes);  // C
  size_t batch_stride = GRID_SIZE * GRID_SIZE * num_channels;  // H * W * C
  std::vector<float> result(annotations_batch.size() * batch_stride);  // NHWC

  // Write the structured annotations into the float buffer.
  float* result_out = result.data();
  for (const std::vector<image_annotation>& annotations : annotations_batch) {

    data_iterator::convert_annotations_to_yolo(
        annotations, GRID_SIZE, GRID_SIZE, NUM_ANCHOR_BOXES, num_classes,
        result_out);

    result_out += batch_stride;
  }

  // Wrap the resulting buffer and return it.
  return shared_float_array::wrap(
      std::move(result),
      { annotations_batch.size(), GRID_SIZE, GRID_SIZE, num_channels });
}

flex_int object_detector::get_max_iterations() const {
  return variant_get_value<flex_int>(state.at("max_iterations"));
}

flex_int object_detector::get_training_iterations() const {
  return variant_get_value<flex_int>(state.at("training_iterations"));
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

}  // object_detection
}  // turi 
