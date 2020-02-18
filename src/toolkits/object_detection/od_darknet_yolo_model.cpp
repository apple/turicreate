/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/object_detection/od_darknet_yolo_model.hpp>

#include <toolkits/object_detection/od_yolo.hpp>

namespace turi {
namespace object_detection {

namespace {

using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::image_augmenter;
using neural_net::model_spec;
using neural_net::Publisher;
using neural_net::shared_float_array;
using neural_net::xavier_weight_initializer;

using padding_type = model_spec::padding_type;

// The spatial reduction depends on the input size of the pre-trained model
// (relative to the grid size).
// TODO: When we support alternative base models, we will have to generalize.
constexpr int SPATIAL_REDUCTION = 32;

constexpr float BASE_LEARNING_RATE = 0.001f;

// Each bounding box is evaluated relative to a list of pre-defined sizes.
const std::vector<std::pair<float, float>>& GetAnchorBoxes() {
  static const std::vector<std::pair<float, float>>* const default_boxes =
      new std::vector<std::pair<float, float>>({
          {1.f, 2.f},
          {1.f, 1.f},
          {2.f, 1.f},
          {2.f, 4.f},
          {2.f, 2.f},
          {4.f, 2.f},
          {4.f, 8.f},
          {4.f, 4.f},
          {8.f, 4.f},
          {8.f, 16.f},
          {8.f, 8.f},
          {16.f, 8.f},
          {16.f, 32.f},
          {16.f, 16.f},
          {32.f, 16.f},
      });
  return *default_boxes;
};

// These are the fixed values that the Python implementation currently passes
// into TCMPS.
// TODO: These should be exposed in a way that facilitates experimentation.
// TODO: A struct instead of a map would be nice, too.

float_array_map GetBaseBackendConfig() {
  float_array_map config;
  config["learning_rate"] = shared_float_array::wrap(BASE_LEARNING_RATE);
  config["gradient_clipping"] = shared_float_array::wrap(0.025f);
  // TODO: Have MPS path use these parameters, instead
  // of the values hardcoded in the MPS code.
  config["od_rescore"] = shared_float_array::wrap(1.0f);
  config["lmb_noobj"] = shared_float_array::wrap(5.0);
  config["lmb_obj"] = shared_float_array::wrap(100.0);
  config["lmb_coord_xy"] = shared_float_array::wrap(10.0);
  config["lmb_coord_wh"] = shared_float_array::wrap(10.0);
  config["lmb_class"] = shared_float_array::wrap(2.0);
  return config;
}

float_array_map GetTrainingBackendConfig(int max_iterations, int num_classes) {
  float_array_map config = GetBaseBackendConfig();
  config["mode"] = shared_float_array::wrap(0.f);
  config["od_include_loss"] = shared_float_array::wrap(1.0f);
  config["od_include_network"] = shared_float_array::wrap(1.0f);
  config["od_max_iou_for_no_object"] = shared_float_array::wrap(0.3f);
  config["od_min_iou_for_object"] = shared_float_array::wrap(0.7f);
  config["rescore"] = shared_float_array::wrap(1.0f);
  config["od_scale_class"] = shared_float_array::wrap(2.0f);
  config["od_scale_no_object"] = shared_float_array::wrap(5.0f);
  config["od_scale_object"] = shared_float_array::wrap(100.0f);
  config["od_scale_wh"] = shared_float_array::wrap(10.0f);
  config["od_scale_xy"] = shared_float_array::wrap(10.0f);
  config["use_sgd"] = shared_float_array::wrap(1.0f);
  config["weight_decay"] = shared_float_array::wrap(0.0005f);
  config["num_iterations"] = shared_float_array::wrap(max_iterations);
  config["num_classes"] = shared_float_array::wrap(num_classes);
  return config;
}

int GetNumOutputChannels(const Config& config) {
  int num_outputs_per_anchor =
      5 + config.num_classes;  // 4 bbox coords + 1 conf + one-hot class labels
  return static_cast<int>(num_outputs_per_anchor * GetAnchorBoxes().size());
}

float_array_map ConvertWeightsExternalToInternal(
    const float_array_map& raw_model_params) {
  // Strip the substring "_fwd" from any parameter names, for compatibility with
  // the compute backend.
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

void InitializeDarknetYOLO(model_spec* nn_spec, int num_classes,
                           int random_seed) {
  // Initialize a random number generator for weight initialization.
  std::seed_seq seed_seq = {random_seed};
  std::mt19937 random_engine(seed_seq);

  // Append conv7, initialized using the Xavier method (with base magnitude 3).
  // The conv7 weights have shape [1024, 1024, 3, 3], so fan in and fan out are
  // both 1024*3*3.
  xavier_weight_initializer conv7_init_fn(1024 * 3 * 3, 1024 * 3 * 3,
                                          &random_engine);
  nn_spec->add_convolution(/* name */ "conv7_fwd",
                           /* input */ "leakyrelu6_fwd",
                           /* num_output_channels */ 1024,
                           /* num_kernel_channels */ 1024,
                           /* kernel_height */ 3,
                           /* kernel_width */ 3,
                           /* stride_height */ 1,
                           /* stride_width */ 1,
                           /* padding */ padding_type::SAME,
                           /* weight_init_fn */ conv7_init_fn);

  // Append batchnorm7.
  nn_spec->add_batchnorm(/* name */ "batchnorm7_fwd",
                         /* input */ "conv7_fwd",
                         /* num_channels */ 1024,
                         /* epsilon */ 0.00001f);

  // Append leakyrelu7.
  nn_spec->add_leakyrelu(/* name */ "leakyrelu7_fwd",
                         /* input */ "batchnorm7_fwd",
                         /* alpha */ 0.1f);

  // Append conv8.
  static constexpr float CONV8_MAGNITUDE = 0.00005f;
  const size_t num_predictions = 5 + num_classes;  // Per anchor box
  const size_t conv8_c_out = GetAnchorBoxes().size() * num_predictions;
  auto conv8_weight_init_fn = [&random_engine](float* w, float* w_end) {
    std::uniform_real_distribution<float> dist(-CONV8_MAGNITUDE,
                                               CONV8_MAGNITUDE);
    while (w != w_end) {
      *w++ = dist(random_engine);
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
  nn_spec->add_convolution(/* name */ "conv8_fwd",
                           /* input */ "leakyrelu7_fwd",
                           /* num_output_channels */ conv8_c_out,
                           /* num_kernel_channels */ 1024,
                           /* kernel_height */ 1,
                           /* kernel_width */ 1,
                           /* stride_height */ 1,
                           /* stride_width */ 1,
                           /* padding */ padding_type::SAME,
                           /* weight_init_fn */ conv8_weight_init_fn,
                           /* bias_init_fn */ conv8_bias_init_fn);
}

}  // namespace

image_augmenter::options DarknetYOLOTrainingAugmentationOptions(
    int batch_size, int output_height, int output_width) {
  image_augmenter::options opts;

  // Specify the fixed image size expected by the neural network.
  opts.batch_size = static_cast<size_t>(batch_size);
  opts.output_height = static_cast<size_t>(output_height * SPATIAL_REDUCTION);
  opts.output_width = static_cast<size_t>(output_width * SPATIAL_REDUCTION);

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

EncodedInputBatch EncodeDarknetYOLO(InputBatch input_batch,
                                    size_t output_height, size_t output_width,
                                    size_t num_anchors, size_t num_classes) {
  EncodedInputBatch result;
  result.iteration_id = input_batch.iteration_id;
  result.images = std::move(input_batch.images);
  result.annotations = std::move(input_batch.annotations);

  // Allocate a float buffer of sufficient size.
  // TODO: Recycle these allocations.
  size_t batch_size = result.images.shape()[0];
  size_t num_channels = num_anchors * (5 + num_classes);  // C
  size_t batch_stride =
      output_height * output_width * num_channels;       // H * W * C
  std::vector<float> buffer(batch_size * batch_stride);  // NHWC

  // Write the structured annotations into the float buffer.
  float* result_out = buffer.data();
  if (result.annotations.size() > batch_size) {
    result.annotations.resize(batch_size);
  }
  for (const std::vector<neural_net::image_annotation>& annotations :
       result.annotations) {
    convert_annotations_to_yolo(annotations, output_height, output_width,
                                num_anchors, num_classes, result_out);

    result_out += batch_stride;
  }

  // Wrap the buffer and put it into the result struct.
  result.labels = neural_net::shared_float_array::wrap(
      std::move(buffer),
      {batch_size, output_height, output_width, num_channels});

  return result;
}

TrainingOutputBatch DarknetYOLOTrainer::Invoke(EncodedInputBatch input_batch) {
  ApplyLearningRateSchedule(input_batch.iteration_id);

  auto results = impl_->train(
      {{"input", input_batch.images}, {"labels", input_batch.labels}});

  TrainingOutputBatch output_batch;
  output_batch.iteration_id = input_batch.iteration_id;
  output_batch.loss = results.at("loss");
  return output_batch;
}

void DarknetYOLOTrainer::ApplyLearningRateSchedule(int iteration_id) {
  // Leave the learning rate unchanged for the first half of the expected number
  // of iterations.
  if (iteration_id == 1 + max_iterations_ / 2) {
    // On the first iteration of the second half, reduce by a factor of 10.
    impl_->set_learning_rate(base_learning_rate_ / 10.f);
  } else if (iteration_id == 1 + max_iterations_ * 3 / 4) {
    // On the first iteration of the last quarter, reduce by another factor of
    // 10.
    impl_->set_learning_rate(base_learning_rate_ / 100.f);
  } else if (iteration_id == 1 + max_iterations_) {
    // Handle any manually triggered iterations after the last planned one.
    impl_->set_learning_rate(base_learning_rate_ / 1000.f);
  }
}

Checkpoint DarknetYOLOCheckpointer::Next() {
  Checkpoint checkpoint;
  checkpoint.config = config_;

  // Copy the weights out from the backend.
  neural_net::float_array_map weights = impl_->export_weights();

  // Convert keys from the model_backend names (e.g. "conv7_weight") to the
  // names in the on-disk representations (e.g. "conv7_fwd_weight").
  for (const auto& kv : weights) {
    const std::string modifier = "_fwd";
    std::string key = kv.first;
    std::string::iterator it = std::find(key.begin(), key.end(), '_');
    key.insert(it, modifier.begin(), modifier.end());
    checkpoint.weights[key] = kv.second;
  }

  return checkpoint;
}

// static
std::unique_ptr<DarknetYOLOModel> DarknetYOLOModel::Create(
    const Config& config, const std::string& pretrained_model_path,
    int random_seed, std::unique_ptr<neural_net::compute_context> context) {
  // Start with parameters from the pre-trained model.
  model_spec nn_spec(pretrained_model_path);

  // Verify that the pre-trained model ends with the expected leakyrelu6 layer.
  // TODO: Also verify that activation shape here is [1024, 13, 13]?
  if (!nn_spec.has_layer_output("leakyrelu6_fwd")) {
    log_and_throw(
        "Expected leakyrelu6_fwd layer in NeuralNetwork parsed from " +
        pretrained_model_path);
  }

  // Append the randomly initialized layers.
  InitializeDarknetYOLO(&nn_spec, config.num_classes, random_seed);

  // Create an initial checkpoint. Note that the weights are a WEAK reference to
  // the model_spec above.
  Checkpoint checkpoint;
  checkpoint.config = config;
  checkpoint.weights = nn_spec.export_params_view();

  // The constructor should copy the weights from the checkpoint, so that it's
  // safe to deallocate the model_spec above.
  // TODO: Avoid weak references like the above.
  std::unique_ptr<DarknetYOLOModel> model;
  model.reset(new DarknetYOLOModel(checkpoint, std::move(context)));
  return model;
}

DarknetYOLOModel::DarknetYOLOModel(
    const Checkpoint& checkpoint,
    std::unique_ptr<neural_net::compute_context> context)
    : Model(context->create_image_augmenter(
          DarknetYOLOTrainingAugmentationOptions(
              checkpoint.config.batch_size, checkpoint.config.output_height,
              checkpoint.config.output_width))),
      config_(checkpoint.config),
      backend_(context->create_object_detector(
          /* n       */ config_.batch_size,
          /* c_in    */ 3,  // RGB input
          /* h_in    */ config_.output_height * SPATIAL_REDUCTION,
          /* w_in    */ config_.output_width * SPATIAL_REDUCTION,
          /* c_out   */ GetNumOutputChannels(config_),
          /* h_out   */ config_.output_height,
          /* w_out   */ config_.output_width,
          /* config  */
          GetTrainingBackendConfig(config_.max_iterations, config_.num_classes),
          /* weights */ ConvertWeightsExternalToInternal(checkpoint.weights))) {
}

std::shared_ptr<Publisher<Checkpoint>>
DarknetYOLOModel::AsCheckpointPublisher() {
  auto checkpointer =
      std::make_shared<DarknetYOLOCheckpointer>(config_, backend_);
  return checkpointer->AsPublisher();
}

std::shared_ptr<Publisher<TrainingOutputBatch>>
DarknetYOLOModel::AsTrainingBatchPublisher(
    std::shared_ptr<neural_net::Publisher<InputBatch>> augmented_data) {
  Config config = config_;

  // Define a lambda that applies EncodeDarknetYOLO to the raw annotations.
  auto encoder = [config](InputBatch input_batch) {
    return EncodeDarknetYOLO(
        std::move(input_batch), config.output_height, config.output_width,
        static_cast<int>(GetAnchorBoxes().size()), config.num_classes);
  };

  // Wrap the model_backend.
  auto trainer = std::make_shared<DarknetYOLOTrainer>(
      backend_, BASE_LEARNING_RATE, config_.max_iterations);

  // Append the encoding function and the model backend to the pipeline.
  // TODO: Dispatch augmentation to a separate thread/queue.
  return augmented_data->Map(encoder)->Map(trainer);
}

}  // namespace object_detection
}  // namespace turi
