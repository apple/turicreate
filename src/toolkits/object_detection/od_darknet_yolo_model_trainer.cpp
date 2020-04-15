/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/object_detection/od_darknet_yolo_model_trainer.hpp>

#include <toolkits/object_detection/od_evaluation.hpp>
#include <toolkits/object_detection/od_serialization.hpp>
#include <toolkits/object_detection/od_yolo.hpp>

namespace turi {
namespace object_detection {

namespace {

using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::image_augmenter;
using neural_net::model_spec;
using neural_net::pipeline_spec;
using neural_net::Publisher;
using neural_net::shared_float_array;
using neural_net::xavier_weight_initializer;

using padding_type = model_spec::padding_type;

// The spatial reduction depends on the input size of the pre-trained model
// (relative to the grid size).
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

std::unique_ptr<model_spec> InitializeDarknetYOLO(
    const std::string& pretrained_model_path, int num_classes,
    int random_seed) {
  // Start with parameters from the pre-trained model.
  std::unique_ptr<model_spec> nn_spec;
  nn_spec.reset(new model_spec(pretrained_model_path));

  // Verify that the pre-trained model ends with the expected leakyrelu6 layer.
  // TODO: Also verify that activation shape here is [1024, 13, 13]?
  if (!nn_spec->has_layer_output("leakyrelu6_fwd")) {
    log_and_throw(
        "Expected leakyrelu6_fwd layer in NeuralNetwork parsed from " +
        pretrained_model_path);
  }

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

  return nn_spec;
}

}  // namespace

image_augmenter::options DarknetYOLOInferenceAugmentationOptions(
    int batch_size, int output_height, int output_width) {
  image_augmenter::options opts;

  // Specify the fixed image size expected by the neural network.
  opts.batch_size = static_cast<size_t>(batch_size);
  opts.output_height = static_cast<size_t>(output_height * SPATIAL_REDUCTION);
  opts.output_width = static_cast<size_t>(output_width * SPATIAL_REDUCTION);
  return opts;
}

image_augmenter::options DarknetYOLOTrainingAugmentationOptions(
    int batch_size, int output_height, int output_width, int random_seed) {
  image_augmenter::options opts = DarknetYOLOInferenceAugmentationOptions(
      batch_size, output_height, output_width);

  opts.random_seed = random_seed;

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
  result.image_sizes = std::move(input_batch.image_sizes);

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

InferenceOutputBatch DecodeDarknetYOLOInference(EncodedBatch batch,
                                                float confidence_threshold,
                                                float iou_threshold) {
  InferenceOutputBatch result;
  result.iteration_id = batch.iteration_id;

  result.predictions.resize(batch.image_sizes.size());
  for (size_t i = 0; i < result.predictions.size(); ++i) {
    // For this row (corresponding to one image), extract the prediction.
    shared_float_array raw_prediction = batch.encoded_data.at("output")[i];

    // Translate the raw output into predicted labels and bounding boxes.
    result.predictions[i] = convert_yolo_to_annotations(
        raw_prediction, GetAnchorBoxes(), confidence_threshold);

    // Remove overlapping predictions.
    result.predictions[i] = apply_non_maximum_suppression(
        std::move(result.predictions[i]), iou_threshold);
  }

  result.annotations = std::move(batch.annotations);
  result.image_sizes = std::move(batch.image_sizes);
  return result;
}

TrainingOutputBatch DarknetYOLOBackendTrainingWrapper::Invoke(
    EncodedInputBatch input_batch) {
  ApplyLearningRateSchedule(input_batch.iteration_id);

  auto results = impl_->train(
      {{"input", input_batch.images}, {"labels", input_batch.labels}});

  TrainingOutputBatch output_batch;
  output_batch.iteration_id = input_batch.iteration_id;
  output_batch.loss = results.at("loss");
  return output_batch;
}

void DarknetYOLOBackendTrainingWrapper::ApplyLearningRateSchedule(
    int iteration_id) {
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

EncodedBatch DarknetYOLOBackendInferenceWrapper::Invoke(
    EncodedInputBatch input_batch) {
  EncodedBatch output_batch;
  output_batch.iteration_id = input_batch.iteration_id;
  output_batch.encoded_data = impl_->predict({{"input", input_batch.images}});
  output_batch.annotations = std::move(input_batch.annotations);
  output_batch.image_sizes = std::move(input_batch.image_sizes);
  return output_batch;
}

std::unique_ptr<Checkpoint> DarknetYOLOCheckpointer::Next() {
  // Copy the weights out from the backend.
  float_array_map backend_weights = impl_->export_weights();

  // Convert keys from the model_backend names (e.g. "conv7_weight") to the
  // names in the on-disk representations (e.g. "conv7_fwd_weight").
  float_array_map weights;
  for (const auto& kv : backend_weights) {
    const std::string modifier = "_fwd";
    std::string key = kv.first;
    std::string::iterator it = std::find(key.begin(), key.end(), '_');
    key.insert(it, modifier.begin(), modifier.end());
    weights[key] = kv.second;
  }

  std::unique_ptr<Checkpoint> checkpoint;
  checkpoint.reset(new DarknetYOLOCheckpoint(config_, std::move(weights)));
  return checkpoint;
}

DarknetYOLOCheckpoint::DarknetYOLOCheckpoint(
    Config config, const std::string& pretrained_model_path)
    : config_(std::move(config)),
      model_spec_(InitializeDarknetYOLO(
          pretrained_model_path, config_.num_classes, config_.random_seed)),
      weights_(model_spec_->export_params_view()) {}

DarknetYOLOCheckpoint::DarknetYOLOCheckpoint(Config config,
                                             float_array_map weights)
    : config_(std::move(config)), weights_(std::move(weights)) {}

const Config& DarknetYOLOCheckpoint::config() const { return config_; }

const float_array_map& DarknetYOLOCheckpoint::weights() const {
  return weights_;
}

std::unique_ptr<ModelTrainer> DarknetYOLOCheckpoint::CreateModelTrainer(
    neural_net::compute_context* context) const {
  std::unique_ptr<DarknetYOLOModelTrainer> result;
  result.reset(new DarknetYOLOModelTrainer(*this, context));
  return result;
}

pipeline_spec DarknetYOLOCheckpoint::ExportToCoreML(
    const std::string& input_name, const std::string& coordinates_output_name,
    const std::string& confidence_output_name) const {
  return export_darknet_yolo(weights_, input_name, coordinates_output_name,
                             confidence_output_name, GetAnchorBoxes(),
                             config_.num_classes, config_.output_height,
                             config_.output_width, SPATIAL_REDUCTION);
}

float_array_map DarknetYOLOCheckpoint::internal_config() const {
  return GetTrainingBackendConfig(config_.max_iterations, config_.num_classes);
}

float_array_map DarknetYOLOCheckpoint::internal_weights() const {
  return ConvertWeightsExternalToInternal(weights_);
}

DarknetYOLOModelTrainer::DarknetYOLOModelTrainer(
    const DarknetYOLOCheckpoint& checkpoint,
    neural_net::compute_context* context)
    : config_(checkpoint.config()),
      backend_(context->create_object_detector(
          /* n       */ config_.batch_size,
          /* c_in    */ 3,  // RGB input
          /* h_in    */ config_.output_height * SPATIAL_REDUCTION,
          /* w_in    */ config_.output_width * SPATIAL_REDUCTION,
          /* c_out   */ GetNumOutputChannels(config_),
          /* h_out   */ config_.output_height,
          /* w_out   */ config_.output_width,
          /* config  */ checkpoint.internal_config(),
          /* weights */ checkpoint.internal_weights())),
      training_augmenter_(
          std::make_shared<DataAugmenter>(context->create_image_augmenter(
              DarknetYOLOTrainingAugmentationOptions(
                  checkpoint.config().batch_size,
                  checkpoint.config().output_height,
                  checkpoint.config().output_width,
                  checkpoint.config().random_seed)))),
      inference_augmenter_(
          std::make_shared<DataAugmenter>(context->create_image_augmenter(
              DarknetYOLOInferenceAugmentationOptions(
                  checkpoint.config().batch_size,
                  checkpoint.config().output_height,
                  checkpoint.config().output_width)))) {}

std::shared_ptr<Publisher<TrainingOutputBatch>>
DarknetYOLOModelTrainer::AsTrainingBatchPublisher(
    std::unique_ptr<data_iterator> training_data, size_t batch_size,
    int offset) {
  // Wrap the data_iterator to incorporate into a Combine pipeline.
  auto iterator = std::make_shared<DataIterator>(std::move(training_data),
                                                 batch_size, offset);

  // Define a lambda that applies EncodeDarknetYOLO to the raw annotations.
  Config config = config_;
  auto encoder = [config](InputBatch input_batch) {
    return EncodeDarknetYOLO(
        std::move(input_batch), config.output_height, config.output_width,
        static_cast<int>(GetAnchorBoxes().size()), config.num_classes);
  };

  // Wrap the model_backend.
  auto trainer = std::make_shared<DarknetYOLOBackendTrainingWrapper>(
      backend_, BASE_LEARNING_RATE, config_.max_iterations);

  // Construct the training pipeline.
  return iterator->AsPublisher()
      ->Map(training_augmenter_)
      ->Map(encoder)
      ->Map(trainer);
}

std::shared_ptr<Publisher<EncodedBatch>>
DarknetYOLOModelTrainer::AsInferenceBatchPublisher(
    std::unique_ptr<data_iterator> test_data, size_t batch_size,
    float confidence_threshold, float iou_threshold) {
  // Wrap the data_iterator to incorporate into a Combine pipeline.
  auto iterator = std::make_shared<DataIterator>(std::move(test_data),
                                                 batch_size, /* offset */ 0);

  // No labels to encode. Just pass the annotations through for potential
  // evaluation.
  auto trivial_encoder = [](InputBatch input_batch) {
    EncodedInputBatch result;
    result.iteration_id = input_batch.iteration_id;
    result.images = std::move(input_batch.images);
    result.annotations = std::move(input_batch.annotations);
    result.image_sizes = std::move(input_batch.image_sizes);
    return result;
  };

  // Wrap the model_backend.
  auto predicter =
      std::make_shared<DarknetYOLOBackendInferenceWrapper>(backend_);

  // Construct the inference pipeline.
  return iterator->AsPublisher()
      ->Map(inference_augmenter_)
      ->Map(trivial_encoder)
      ->Map(predicter);
}

InferenceOutputBatch DarknetYOLOModelTrainer::DecodeOutputBatch(
    EncodedBatch batch, float confidence_threshold, float iou_threshold) {
  return DecodeDarknetYOLOInference(std::move(batch), confidence_threshold,
                                    iou_threshold);
}

std::shared_ptr<Publisher<std::unique_ptr<Checkpoint>>>
DarknetYOLOModelTrainer::AsCheckpointPublisher() {
  auto checkpointer =
      std::make_shared<DarknetYOLOCheckpointer>(config_, backend_);
  return checkpointer->AsPublisher();
}

// TODO: Remove this method. It is only called by the base class implementation
// of AsTrainingBatchPublisher we overrode above.
std::shared_ptr<Publisher<TrainingOutputBatch>>
DarknetYOLOModelTrainer::AsTrainingBatchPublisher(
    std::shared_ptr<neural_net::Publisher<InputBatch>> augmented_data) {
  Config config = config_;

  // Define a lambda that applies EncodeDarknetYOLO to the raw annotations.
  auto encoder = [config](InputBatch input_batch) {
    return EncodeDarknetYOLO(
        std::move(input_batch), config.output_height, config.output_width,
        static_cast<int>(GetAnchorBoxes().size()), config.num_classes);
  };

  // Wrap the model_backend.
  auto trainer = std::make_shared<DarknetYOLOBackendTrainingWrapper>(
      backend_, BASE_LEARNING_RATE, config_.max_iterations);

  // Append the encoding function and the model backend to the pipeline.
  // TODO: Dispatch augmentation to a separate thread/queue.
  return augmented_data->Map(encoder)->Map(trainer);
}

}  // namespace object_detection
}  // namespace turi
