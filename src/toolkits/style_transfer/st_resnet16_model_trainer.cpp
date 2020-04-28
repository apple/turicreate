/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/style_transfer/st_resnet16_model_trainer.hpp>

#include <toolkits/style_transfer/style_transfer_model_definition.hpp>

namespace turi {
namespace style_transfer {

using neural_net::compute_context;
using neural_net::CreatePublisherFromCallable;
using neural_net::float_array_map;
using neural_net::model_backend;
using neural_net::model_spec;
using neural_net::Publisher;
using neural_net::shared_float_array;

namespace {

std::unique_ptr<model_spec> CreateSpec(const Config& config,
                                       const std::string& resnet_mlmodel_path) {
  if (resnet_mlmodel_path.empty()) {
    return init_resnet(config.num_styles, config.random_seed);
  } else {
    return init_resnet(resnet_mlmodel_path, config.num_styles);
  }
}

}  // namespace

ResNet16Checkpoint::ResNet16Checkpoint(Config config,
                                       const std::string& resnet_mlmodel_path)
    : Checkpoint(config,
                 ExtractWeights(CreateSpec(config, resnet_mlmodel_path))) {}

ResNet16Checkpoint::ResNet16Checkpoint(Config config, float_array_map weights)
    : Checkpoint(std::move(config), std::move(weights)) {}

std::unique_ptr<ModelTrainer> ResNet16Checkpoint::CreateModelTrainer() const {
  std::unique_ptr<ModelTrainer> result;
  result.reset(new ResNet16ModelTrainer(config(), weights()));
  return result;
}

model_spec ResNet16Checkpoint::ExportToCoreML() const {
  std::unique_ptr<model_spec> resnet_spec = init_resnet(config().num_styles);
  resnet_spec->update_params(weights());
  return std::move(*resnet_spec);
}

ResNet16ModelTrainer::ResNet16ModelTrainer(Config config,
                                           float_array_map weights)
    : ModelTrainer(std::move(config)), state_(std::make_shared<ModelState>()) {
  state_->weights = std::move(weights);
}

std::shared_ptr<Publisher<std::unique_ptr<Checkpoint>>>
ResNet16ModelTrainer::AsCheckpointPublisher() {
  std::shared_ptr<Config> config = std::make_shared<Config>(this->config());
  std::shared_ptr<ModelState> state = state_;
  auto impl = [config, state] {
    std::unique_ptr<Checkpoint> checkpoint;
    checkpoint.reset(new ResNet16Checkpoint(*config, GetWeights(*state)));
    return checkpoint;
  };
  return CreatePublisherFromCallable(impl);
}

std::shared_ptr<model_backend> ResNet16ModelTrainer::CreateTrainingBackend(
    const std::string& vgg_mlmodel_path, compute_context* context) {
  float_array_map config = {
      // A value of `1` to indicate training
      {"st_training", shared_float_array::wrap(1.f)},
      {"st_num_styles", shared_float_array::wrap(this->config().num_styles)}};

  std::unique_ptr<model_spec> vgg_spec = init_vgg_16(vgg_mlmodel_path);
  float_array_map weights = GetWeights(*state_);
  float_array_map vgg_weights = vgg_spec->export_params_view();
  weights.insert(vgg_weights.begin(), vgg_weights.end());

  // Save a reference to the training backend to use as a source of weights for
  // creating checkpoints and inference backends.
  state_->training_backend = context->create_style_transfer(config, weights);
  state_->weights.clear();  // No longer used

  return state_->training_backend;
}

std::shared_ptr<model_backend> ResNet16ModelTrainer::CreateInferenceBackend(
    compute_context* context) {
  float_array_map config = {
      // A value of `0` to indicate prediction
      {"st_training", shared_float_array::wrap(0.f)},
      {"st_num_styles", shared_float_array::wrap(this->config().num_styles)}};
  return context->create_style_transfer(config, GetWeights(*state_));
}

// static
float_array_map ResNet16ModelTrainer::GetWeights(const ModelState& state) {
  if (state.training_backend != nullptr) {
    return state.training_backend->export_weights();
  } else {
    return state.weights;
  }
}

}  // namespace style_transfer
}  // namespace turi
