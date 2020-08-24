/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TOOLKITS_OBJECT_DETECTION_OD_MODEL_TRAINER_HPP_
#define TOOLKITS_OBJECT_DETECTION_OD_MODEL_TRAINER_HPP_

/**
 * \file od_model_trainer.hpp
 *
 * Defines the value types representing each stage of an object-detection
 * training pipeline, and the virtual interface for arbitrary object-detection
 * models.
 */

#include <ml/neural_net/combine.hpp>
#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <toolkits/object_detection/od_data_iterator.hpp>

namespace turi {
namespace object_detection {

class ModelTrainer;

/** Represents one batch of raw data: (possibly) annotated images. */
struct DataBatch {
  /** The serial number for this batch, starting with 1. */
  int iteration_id = 0;

  std::vector<neural_net::labeled_image> examples;
};

/** Represents one batch of model-agnostic data, post-augmentation/resizing. */
struct InputBatch {
  int iteration_id = 0;

  // TODO: Adopt NCHW.
  /** The (RGB) images from a DataBatch encoded as NHWC. */
  neural_net::shared_float_array images;

  /** The raw annotations from the DataBatch. */
  std::vector<std::vector<neural_net::image_annotation>> annotations;

  /**
   * The original height and width of each image, used to scale bounding-box
   * predictions.
   */
  std::vector<std::pair<size_t, size_t>> image_sizes;
};

/** Represents one batch of data, in a possibly model-specific format. */
struct EncodedInputBatch {
  int iteration_id = 0;

  // TODO: Migrate to neural_net::float_array_map
  neural_net::shared_float_array images;
  neural_net::shared_float_array labels;

  // The raw annotations are preserved to support evaluation, comparing raw
  // annotations against model predictions.
  std::vector<std::vector<neural_net::image_annotation>> annotations;

  // The original image sizes are preserved to support prediction.
  std::vector<std::pair<size_t, size_t>> image_sizes;
};

/** Represents the raw output of an object-detection model. */
// TODO: Adopt EncodedBatch instead.
struct TrainingOutputBatch {
  int iteration_id = 0;
  neural_net::shared_float_array loss;
};

/** Represents the output conveyed to the user. */
struct TrainingProgress {
  int iteration_id = 0;
  float smoothed_loss = 0.f;
};

/**
 * Represents the immediate (model-specific) input or output of a model backend,
 * using the float_array_map representation.
 */
struct EncodedBatch {
  int iteration_id = 0;

  neural_net::float_array_map encoded_data;

  std::vector<std::vector<neural_net::image_annotation>> annotations;
  std::vector<std::pair<size_t, size_t>> image_sizes;
};

/** Represents one batch of inference results, in a generic format. */
struct InferenceOutputBatch {
  int iteration_id = 0;

  std::vector<std::vector<neural_net::image_annotation>> predictions;

  std::vector<std::vector<neural_net::image_annotation>> annotations;
  std::vector<std::pair<size_t, size_t>> image_sizes;
};

/** Ostensibly model-agnostic parameters for object detection. */
struct Config {
  /**
   * The target number of training iterations to perform.
   *
   * If -1, then this target should be computed heuristically.
   */
  int max_iterations = -1;

  /**
   * The number of images to process per training batch.
   *
   * If -1, then this size should be computed automatically.
   */
  int batch_size = -1;

  /** For darknet-yolo, the height of the final feature map. */
  int output_height = 13;

  /** For darknet-yolo, the width of the final feature map. */
  int output_width = 13;

  /** Determines the number of feature channels in the final feature map. */
  int num_classes = -1;
};

/** Stores additional data for specific model backend for a checkpoint. */
struct CheckpointMetadata {
  /** The number of predictions for the loaded model. */
  size_t num_predictions = 0;

  /** The model type name for use in exported models. */
  std::string model_type = "";

  /** The confidence threshold for evaluation */
  float evaluate_confidence = 0.f;

  /** The confidence threshold for prediction */
  float predict_confidence = 0.f;

  /** The Non Maximal Suppression threshold for evaluation */
  float nms_threshold = 0.f;

  /** When true, use NMS only on the most confident class otherwise across all classes. */
  bool use_most_confident_class = false;
};

/**
 * A representation of all the parameters needed to reconstruct a model.
 *
 * \todo Include optimizer state to allow training to resume seamlessly.
 */
class Checkpoint {
 public:
  virtual ~Checkpoint() = default;

  virtual const Config& config() const = 0;
  virtual const neural_net::float_array_map& weights() const = 0;

  /** Loads the checkpoint into an active ModelTrainer instance. */
  virtual std::unique_ptr<ModelTrainer> CreateModelTrainer(
      neural_net::compute_context* context) const = 0;

  /**
   * Returns the CoreML spec corresponding to the current model.
   *
   * The result must be a pipeline that accepts an image input and yields at
   * least two outputs, all with the given names. The outputs must be suitable
   * for passing directly into a NonMaximumSuppression model.
   */
  virtual neural_net::pipeline_spec ExportToCoreML(const std::string& input_name,
                                                   const std::string& coordinates_name,
                                                   const std::string& confidence_name,
                                                   bool use_nms_layer, float iou_threshold,
                                                   float confidence_threshold) const = 0;

  virtual CheckpointMetadata GetCheckpointMetadata() const = 0;
};
/**
 * Wrapper adapting object_detection::data_iterator to the Iterator interface.
 */
class DataIterator : public neural_net::Iterator<DataBatch> {
 public:
  /**
   * \param impl The object_detection::data_iterator to wrap
   * \param batch_size The number of images to request from impl for each batch.
   * \param offset The number of batches to skip. The first batch produced will
   *     have an iteration_id one more than the offset.
   *
   * \todo object_detection::data_iterator needs to support specifying the
   *     offset (and doing the right thing with random seeding)
   */
  DataIterator(std::unique_ptr<data_iterator> impl, size_t batch_size,
               int offset = 0)
      : impl_(std::move(impl)),
        batch_size_(batch_size),
        last_iteration_id_(offset) {}

  bool HasNext() const override { return impl_->has_next_batch(); }

  DataBatch Next() override;

 private:
  std::unique_ptr<data_iterator> impl_;
  size_t batch_size_ = 32;
  int last_iteration_id_ = 0;  // Next ID starts at 1, not 0, by default.
};

/** Wrapper adapting image_augmenter to the Transform interface. */
class DataAugmenter : public neural_net::Transform<DataBatch, InputBatch> {
 public:
  DataAugmenter(std::unique_ptr<neural_net::image_augmenter> impl)
      : impl_(std::move(impl)) {}

  InputBatch Invoke(DataBatch data_batch) override;

 private:
  std::unique_ptr<neural_net::image_augmenter> impl_;
};

/**
 * Converts raw training output to user-visible progress updates.
 *
 * \todo Adopt this operator once model_backend supports an async API that would
 * avoid performance regressions due to premature waiting on the futures that
 * the model_backend implementations currently output.
 */
class ProgressUpdater
    : public neural_net::Transform<TrainingOutputBatch, TrainingProgress> {
 public:
  ProgressUpdater(std::unique_ptr<float> smoothed_loss)
      : smoothed_loss_(std::move(smoothed_loss)) {}

  TrainingProgress Invoke(TrainingOutputBatch output_batch) override;

 private:
  std::unique_ptr<float> smoothed_loss_;
};

/**
 * Abstract base class for object-detection model trainers.
 *
 * Responsible for constructing the model-agnostic portions of the overall
 * training pipeline.
 */
class ModelTrainer {
 public:
  ModelTrainer() : ModelTrainer(nullptr) {}

  // TODO: This class should be responsible for producing the augmenter itself.
  ModelTrainer(std::unique_ptr<neural_net::image_augmenter> augmenter);

  virtual ~ModelTrainer() = default;

  /**
   * Given a data iterator, return a publisher of model outputs.
   *
   * \todo Eventually this should return a TrainingProgress publisher.
   */
  virtual std::shared_ptr<neural_net::Publisher<TrainingOutputBatch>>
  AsTrainingBatchPublisher(std::unique_ptr<data_iterator> training_data,
                           size_t batch_size, int offset);

  /**
   * Given a data iterator, return a publisher of inference model outputs.
   *
   * \todo Publish InferenceOutputBatch instead of EncodedBatch.
   */
  virtual std::shared_ptr<neural_net::Publisher<EncodedBatch>>
  AsInferenceBatchPublisher(std::unique_ptr<data_iterator> test_data,
                            size_t batch_size, float confidence_threshold,
                            float iou_threshold) = 0;

  /**
   * Convert the raw output of the inference batch publisher into structured
   * predictions.
   *
   * \todo This conversion should be incorporated into the inference pipeline
   *       once the backends support proper asynchronous complete handlers.
   */
  virtual InferenceOutputBatch DecodeOutputBatch(EncodedBatch batch,
                                                 float confidence_threshold,
                                                 float iou_threshold) = 0;

  /** Returns a publisher that can be used to request checkpoints. */
  virtual std::shared_ptr<neural_net::Publisher<std::unique_ptr<Checkpoint>>>
  AsCheckpointPublisher() = 0;

 protected:
  // Used by subclasses to produce the model-specific portions of the overall
  // training pipeline.
  // TODO: Remove this method. Just let subclasses define the entire training
  // pipeline.
  virtual std::shared_ptr<neural_net::Publisher<TrainingOutputBatch>>
  AsTrainingBatchPublisher(
      std::shared_ptr<neural_net::Publisher<InputBatch>> augmented_data) = 0;

 private:
  std::shared_ptr<DataAugmenter> augmenter_;
};

}  // namespace object_detection
}  // namespace turi

#endif  // TOOLKITS_OBJECT_DETECTION_OD_MODEL_TRAINER_HPP_
