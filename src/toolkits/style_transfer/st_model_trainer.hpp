/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TOOLKITS_STYLE_TRANSFER_ST_MODEL_TRAINER_HPP_
#define TOOLKITS_STYLE_TRANSFER_ST_MODEL_TRAINER_HPP_

/**
 * \file st_model_trainer.hpp
 *
 * Defines the value types representing each stage of a style-transfer training
 * pipeline, and the virtual interface for arbitrary style-transfer models.
 */

#include <ml/neural_net/combine.hpp>
#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <toolkits/style_transfer/style_transfer_data_iterator.hpp>

namespace turi {
namespace style_transfer {

class ModelTrainer;

/**
 * Represents one batch of content/style image pairs.
 *
 * Also used for inference, in which case the "style" image is the stylized
 * output.
 */
struct DataBatch {
  /** The serial number for this batch, starting with 1. */
  int iteration_id = 0;

  std::vector<st_example> examples;
};

/**
 * Represents the immediate (model-specific) input or output of a model backend,
 * using the generic float_array_map representation.
 *
 * \todo Define types for input and output batches that don't rely on the
 *       arbitrary keys in float_array_map.
 */
struct EncodedBatch {
  int iteration_id = 0;

  neural_net::float_array_map encoded_data;
};

/** EncodedBatch that also records the style index used for inference. */
struct EncodedInferenceBatch : public EncodedBatch {
  int style_index = -1;
};

/** Represents the output conveyed to the user. */
struct TrainingProgress {
  int iteration_id = 0;

  float smoothed_loss = 0.f;

  // These are only set if the ModelTrainer returns true for
  // SupportsLossComponents().
  // TODO: Should these also be smoothed?
  float style_loss = 0.f;
  float content_loss = 0.f;
};

/** Model-agnostic parameters for style transfer. */
struct Config {
  /** Determines the number of style images used during training. */
  int num_styles = 1;

  /**
   * The target number of training iterations to perform.
   *
   * If -1, then this target should be computed heuristically.
   */
  int max_iterations = -1;

  /** The number of images to process per training batch. */
  int batch_size = 1;

  /** The height of images passed into the training backend. */
  int training_image_height = 256;

  /** The width of images passed into the training backend. */
  int training_image_width = 256;

  /** Random seed used to initialize the model. */
  int random_seed = 0;
};

/**
 * Wrapper adapting style_transfer::data_iterator to the Iterator interface.
 */
class DataIterator : public neural_net::Iterator<DataBatch> {
 public:
  /**
   * \param impl The style_transfer::data_iterator to wrap
   * \param batch_size The number of images to request from impl for each batch.
   * \param offset The number of batches to skip. The first batch produced will
   *     have an iteration_id one more than the offset.
   *
   * \todo style_transfer::data_iterator needs to support specifying the
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
  size_t batch_size_ = 1;
  int last_iteration_id_ = 0;  // Next ID starts at 1, not 0, by default.
};

/**
 * Wrapper around DataIterator that duplicates each batch, with each duplicate
 * writing a different style index into every example for each duplicate.
 */
class InferenceDataIterator : public neural_net::Iterator<DataBatch> {
 public:
  InferenceDataIterator(std::shared_ptr<DataIterator> base_iterator,
                        std::vector<int> style_indices);

  bool HasNext() const override;
  DataBatch Next() override;

 private:
  std::shared_ptr<DataIterator> base_iterator_;
  std::vector<int> style_indices_;
  std::vector<int>::const_iterator next_style_;
  DataBatch current_batch_;
};

/**
 * Converts raw training output to user-visible progress updates.
 */
class ProgressUpdater
    : public neural_net::Transform<EncodedBatch, TrainingProgress> {
 public:
  ProgressUpdater(std::unique_ptr<float> smoothed_loss)
      : smoothed_loss_(std::move(smoothed_loss)) {}

  TrainingProgress Invoke(EncodedBatch output_batch) override;

 private:
  std::unique_ptr<float> smoothed_loss_;
};

/**
 * A representation of all the parameters needed to reconstruct a model.
 *
 * \todo Include optimizer state to allow training to resume seamlessly.
 */
class Checkpoint {
 public:
  Checkpoint(Config config, neural_net::float_array_map weights)
      : config_(std::move(config)), weights_(std::move(weights)) {}

  virtual ~Checkpoint() = default;

  const Config& config() const { return config_; }
  const neural_net::float_array_map& weights() const { return weights_; }

  /** Loads the checkpoint into an active ModelTrainer instance. */
  virtual std::unique_ptr<ModelTrainer> CreateModelTrainer() const = 0;

  /**
   * Returns the CoreML spec corresponding to the current model.
   *
   * The first layer of the model should have a single input: the image to
   * stylize. The last layer of the model should have a single output: the
   * stylized image.
   */
  virtual neural_net::model_spec ExportToCoreML() const = 0;

 protected:
  static neural_net::float_array_map ExtractWeights(
      std::unique_ptr<neural_net::model_spec> nn_spec);

 private:
  Config config_;
  neural_net::float_array_map weights_;
};

/**
 * Abstract base class for style-transfer model trainers.
 *
 * Responsible for constructing the model-agnostic portions of the overall
 * training pipeline.
 */
class ModelTrainer {
 public:
  ModelTrainer(Config config) : config_(std::move(config)) {}

  virtual ~ModelTrainer() = default;

  const Config& config() const { return config_; }

  /**
   * Returns true iff the output from the training batch publisher sets the
   * style_loss and content_loss values.
   */
  virtual bool SupportsLossComponents() const = 0;

  /** Given a data iterator, return a publisher of training model outputs. */
  virtual std::shared_ptr<neural_net::Publisher<TrainingProgress>>
  AsTrainingBatchPublisher(std::unique_ptr<data_iterator> training_data,
                           const std::string& vgg_mlmodel_path, int offset,
                           std::unique_ptr<float> initial_training_loss,
                           neural_net::compute_context* context);

  /** Given a data iterator, return a publisher of inference model outputs. */
  virtual std::shared_ptr<neural_net::Publisher<DataBatch>>
  AsInferenceBatchPublisher(std::unique_ptr<data_iterator> test_data,
                            std::vector<int> style_indices,
                            neural_net::compute_context* context);

  /** Returns a publisher that can be used to request checkpoints. */
  virtual std::shared_ptr<neural_net::Publisher<std::unique_ptr<Checkpoint>>>
  AsCheckpointPublisher() = 0;

 protected:
  // TODO: Style transfer backends should support both training and inference.
  // Then we would only need one.
  virtual std::shared_ptr<neural_net::model_backend> CreateTrainingBackend(
      const std::string& vgg_mlmodel_path,
      neural_net::compute_context* context) = 0;
  virtual std::shared_ptr<neural_net::model_backend> CreateInferenceBackend(
      neural_net::compute_context* context) = 0;

 private:
  Config config_;
};

/**
 * Converts native images into tensors that can be fed into the model backend.
 */
EncodedBatch EncodeTrainingBatch(DataBatch batch, int width, int height);

/**
 * Converts native images into tensors that can be fed into the model backend.
 */
EncodedInferenceBatch EncodeInferenceBatch(DataBatch batch);

/**
 * Converts the raw output from an inference backend into images.
 */
DataBatch DecodeInferenceBatch(EncodedInferenceBatch batch);

}  // namespace style_transfer
}  // namespace turi

#endif  // TOOLKITS_STYLE_TRANSFER_ST_MODEL_TRAINER_HPP_
