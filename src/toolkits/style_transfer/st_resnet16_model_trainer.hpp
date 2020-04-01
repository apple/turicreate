/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TOOLKITS_STYLE_TRANSFER_ST_RESNET16_MODEL_TRAINER_HPP_
#define TOOLKITS_STYLE_TRANSFER_ST_RESNET16_MODEL_TRAINER_HPP_

#include <toolkits/style_transfer/st_model_trainer.hpp>

namespace turi {
namespace style_transfer {

/**
 * Subclass of Checkpoint that generates ResNet16ModelTrainer instances.
 */
class ResNet16Checkpoint : public Checkpoint {
 public:
  /**
   * Loads a pretrained model to use as a starting point.
   */
  ResNet16Checkpoint(Config config, const std::string& resnet_mlmodel_path);

  /** Loads weights saved from a ResNet16ModelTrainer. */
  ResNet16Checkpoint(Config config, neural_net::float_array_map weights);

  std::unique_ptr<ModelTrainer> CreateModelTrainer() const override;

  neural_net::model_spec ExportToCoreML() const override;
};

/** Subclass of ModelTrainer encapsulating the resnet-16 architecture. */
class ResNet16ModelTrainer : public ModelTrainer {
 public:
  /**
   * Initializes a model from a checkpoint.
   */
  ResNet16ModelTrainer(Config config, neural_net::float_array_map weights);

  bool SupportsLossComponents() const override { return false; }

  /** Returns a publisher that can be used to request checkpoints. */
  std::shared_ptr<neural_net::Publisher<std::unique_ptr<Checkpoint>>>
  AsCheckpointPublisher() override;

 protected:
  std::shared_ptr<neural_net::model_backend> CreateTrainingBackend(
      const std::string& vgg_mlmodel_path,
      neural_net::compute_context* context) override;

  std::shared_ptr<neural_net::model_backend> CreateInferenceBackend(
      neural_net::compute_context* context) override;

 private:
  struct ModelState {
    // Non-null if a training backend has been created.
    std::shared_ptr<neural_net::model_backend> training_backend;

    // Only used until a training backend is created.
    neural_net::float_array_map weights;
  };

  static neural_net::float_array_map GetWeights(const ModelState& state);

  // This state is shared with the publishers we create
  std::shared_ptr<ModelState> state_;
};

}  // namespace style_transfer
}  // namespace turi

#endif  // TOOLKITS_STYLE_TRANSFER_ST_RESNET16_MODEL_TRAINER_HPP_
