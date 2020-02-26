/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TOOLKITS_OBJECT_DETECTION_OD_DARKNET_YOLO_MODEL_TRAINER_HPP_
#define TOOLKITS_OBJECT_DETECTION_OD_DARKNET_YOLO_MODEL_TRAINER_HPP_

/**
 * \file od_darknet_yolo_model_trainer.hpp
 *
 * Defines helper functions and the Model subclass for the darknet-yolo
 * architecture.
 */

#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <toolkits/object_detection/od_model_trainer.hpp>

namespace turi {
namespace object_detection {

/** Configures an image_augmenter given darknet-yolo network parameters. */
neural_net::image_augmenter::options DarknetYOLOTrainingAugmentationOptions(
    int batch_size, int output_height, int output_width);

/** 
 * Encodes the annotations of an input batch into the format expected by the
 * darknet-yolo network.
 */
EncodedInputBatch EncodeDarknetYOLO(InputBatch input_batch,
                                    size_t output_height, size_t output_width,
                                    size_t num_anchors, size_t num_classes);

/**
 * Wrapper that integrates a darknet-yolo model_backend into a training
 * pipeline.
 *
 * \todo Once model_backend exposes support for explicit asynchronous
 * invocations, this class won't be able to simply use the Transform base class.
 */
class DarknetYOLOTrainer
    : public neural_net::Transform<EncodedInputBatch, TrainingOutputBatch> {
 public:
  // Uses base_learning_rate and max_iterations to determine the learning-rate
  // schedule.
  DarknetYOLOTrainer(std::shared_ptr<neural_net::model_backend> impl,
                     float base_learning_rate, int max_iterations)
      : impl_(std::move(impl)),
        base_learning_rate_(base_learning_rate),
        max_iterations_(max_iterations) {}

  TrainingOutputBatch Invoke(EncodedInputBatch input_batch) override;

 private:
  void ApplyLearningRateSchedule(int iteration_id);

  std::shared_ptr<neural_net::model_backend> impl_;
  float base_learning_rate_ = 0.f;
  int max_iterations_ = 0;
};

/**
 * Wrapper for a darknet-yolo model_backend that publishes checkpoints.
 */
class DarknetYOLOCheckpointer
    : public neural_net::Iterator<std::unique_ptr<Checkpoint>> {
 public:
  DarknetYOLOCheckpointer(const Config& config,
                          std::shared_ptr<neural_net::model_backend> impl)
      : config_(config), impl_(std::move(impl)) {}

  bool HasNext() const override { return impl_ != nullptr; }

  std::unique_ptr<Checkpoint> Next() override;

 private:
  Config config_;
  std::shared_ptr<neural_net::model_backend> impl_;
};

/**
 * Subclass of Checkpoint that generates DarknetYOLOModelTrainer
 * instances.
 */
class DarknetYOLOCheckpoint : public Checkpoint {
 public:
  /**
   * Initializes a new model, combining the pre-trained warm-start weights with
   * random initialization for the final layers.
   */
  DarknetYOLOCheckpoint(Config config, const std::string& pretrained_model_path,
                        int random_seed);

  /** Loads weights saved from a DarknetYOLOModelTrainer. */
  DarknetYOLOCheckpoint(Config config, neural_net::float_array_map weights);

  const Config& config() const override;
  const neural_net::float_array_map& weights() const override;

  std::unique_ptr<ModelTrainer> CreateModelTrainer(
      neural_net::compute_context* context) const override;

  neural_net::pipeline_spec ExportToCoreML(
      const std::string& input_name, const std::string& coordinates_output_name,
      const std::string& confidence_output_name) const override;

  /** Returns the config dictionary used to initialize darknet-yolo backends. */
  neural_net::float_array_map internal_config() const;

  /** Returns the weights with the keys expected by the backends. */
  neural_net::float_array_map internal_weights() const;

 private:
  Config config_;

  std::unique_ptr<neural_net::model_spec> model_spec_;
  neural_net::float_array_map weights_;
};

/** Subclass of ModelTrainer encapsulating the darknet-yolo architecture. */
class DarknetYOLOModelTrainer : public ModelTrainer {
 public:
  /**
   * Initializes a model from a checkpoint.
   */
  DarknetYOLOModelTrainer(const DarknetYOLOCheckpoint& checkpoint,
                          neural_net::compute_context* context);

  std::shared_ptr<neural_net::Publisher<std::unique_ptr<Checkpoint>>>
  AsCheckpointPublisher() override;

 protected:
  std::shared_ptr<neural_net::Publisher<TrainingOutputBatch>>
  AsTrainingBatchPublisher(std::shared_ptr<neural_net::Publisher<InputBatch>>
                               augmented_data) override;

 private:
  Config config_;
  std::shared_ptr<neural_net::model_backend> backend_;
};

}  // namespace object_detection
}  // namespace turi

#endif  // TOOLKITS_OBJECT_DETECTION_OD_DARKNET_YOLO_MODEL_TRAINER_HPP_
