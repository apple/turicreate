/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TOOLKITS_OBJECT_DETECTION_OD_DARKNET_YOLO_MODEL_HPP_
#define TOOLKITS_OBJECT_DETECTION_OD_DARKNET_YOLO_MODEL_HPP_

/**
 * \file od_darknet_yolo_model.hpp
 *
 * Defines helper functions and the Model subclass for the darknet-yolo
 * architecture.
 */

#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <toolkits/object_detection/od_model.hpp>

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
class DarknetYOLOCheckpointer : public neural_net::Iterator<Checkpoint> {
 public:
  DarknetYOLOCheckpointer(const Config& config,
                          std::shared_ptr<neural_net::model_backend> impl)
      : config_(config), impl_(std::move(impl)) {}

  bool HasNext() const override { return impl_ != nullptr; }

  Checkpoint Next() override;

 private:
  Config config_;
  std::shared_ptr<neural_net::model_backend> impl_;
};

/** Subclass of Model encapsulating the darknet-yolo architecture. */
class DarknetYOLOModel : public Model {
 public:
  /**
   * Initializes a new model, combining the pre-trained warm-start weights with
   * random initialization for the final layers.
   */
  static std::unique_ptr<DarknetYOLOModel> Create(
      const Config& config, const std::string& pretrained_model_path,
      int random_seed, std::unique_ptr<neural_net::compute_context> context);

  /**
   * Initializes a model from a checkpoint.
   */
  DarknetYOLOModel(const Checkpoint& checkpoint,
                   std::unique_ptr<neural_net::compute_context> context);

  std::shared_ptr<neural_net::Publisher<Checkpoint>> AsCheckpointPublisher()
      override;

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

#endif  // TOOLKITS_OBJECT_DETECTION_OD_DARKNET_YOLO_MODEL_HPP_
