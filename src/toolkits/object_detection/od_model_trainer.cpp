/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/object_detection/od_model_trainer.hpp>

namespace turi {
namespace object_detection {

using neural_net::image_augmenter;
using neural_net::labeled_image;
using neural_net::Publisher;

DataBatch DataIterator::Next() {
  DataBatch batch;
  batch.iteration_id = ++last_iteration_id_;
  batch.examples = impl_->next_batch(batch_size_);
  return batch;
}

InputBatch DataAugmenter::Invoke(DataBatch data_batch) {
  InputBatch batch;
  batch.iteration_id = data_batch.iteration_id;

  // Extract the image sizes from data_batch.examples before we move the
  // examples into the augmenter.
  batch.image_sizes.resize(data_batch.examples.size());
  auto extract_size = [](const labeled_image &example) {
    return std::make_pair(example.image.m_height, example.image.m_width);
  };
  std::transform(data_batch.examples.begin(), data_batch.examples.end(),
                 batch.image_sizes.begin(), extract_size);

  image_augmenter::result result =
      impl_->prepare_images(std::move(data_batch.examples));
  batch.images = std::move(result.image_batch);
  batch.annotations = std::move(result.annotations_batch);

  return batch;
}

TrainingProgress ProgressUpdater::Invoke(TrainingOutputBatch output_batch) {
  // Compute the loss for this batch.
  float batch_loss =
      std::accumulate(output_batch.loss.data(),
                      output_batch.loss.data() + output_batch.loss.size(), 0.f,
                      std::plus<float>());

  // Update our rolling average (smoothed) loss.
  if (smoothed_loss_) {
    *smoothed_loss_ *= 0.9f;
    *smoothed_loss_ += 0.1f * batch_loss;
  } else {
    // Initialize smoothed loss to the first loss value.
    smoothed_loss_.reset(new float(batch_loss));
  }

  TrainingProgress progress;
  progress.iteration_id = output_batch.iteration_id;
  progress.smoothed_loss = *smoothed_loss_;
  return progress;
}

ModelTrainer::ModelTrainer(
    std::unique_ptr<neural_net::image_augmenter> augmenter)
    : augmenter_(std::make_shared<DataAugmenter>(std::move(augmenter))) {}

std::shared_ptr<Publisher<TrainingOutputBatch>>
ModelTrainer::AsTrainingBatchPublisher(
    std::unique_ptr<data_iterator> training_data, size_t batch_size,
    int offset) {
  // Wrap the data_iterator to incorporate into a Combine pipeline.
  auto iterator = std::make_shared<DataIterator>(std::move(training_data),
                                                 batch_size, offset);

  // Apply augmentation to the output of the data iterator.
  auto augmented = iterator->AsPublisher()->Map(augmenter_);

  // Pass the result to the model-specific portion of the pipeline, defined by
  // the subclass.
  return AsTrainingBatchPublisher(std::move(augmented));
}

}  // namespace object_detection
}  // namespace turi
