/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/style_transfer/st_model_trainer.hpp>

#include <algorithm>

#include <model_server/lib/image_util.hpp>
#include <toolkits/style_transfer/style_transfer.hpp>

namespace turi {
namespace style_transfer {

using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::model_backend;
using neural_net::model_spec;
using neural_net::Publisher;
using neural_net::shared_float_array;

DataBatch DataIterator::Next() {
  DataBatch batch;
  batch.iteration_id = ++last_iteration_id_;
  batch.examples = impl_->next_batch(batch_size_);
  return batch;
}

InferenceDataIterator::InferenceDataIterator(
    std::shared_ptr<DataIterator> base_iterator, std::vector<int> style_indices)
    : base_iterator_(std::move(base_iterator)),
      style_indices_(std::move(style_indices)),
      next_style_(style_indices_.end()) {}

bool InferenceDataIterator::HasNext() const {
  return next_style_ != style_indices_.end() || base_iterator_->HasNext();
}

DataBatch InferenceDataIterator::Next() {
  // If we're done emitting all the styles for the current underlying batch,
  // fetch the next batch from the underlying data iterator.
  if (next_style_ == style_indices_.end() && base_iterator_->HasNext()) {
    current_batch_ = base_iterator_->Next();
    next_style_ = style_indices_.begin();
  }

  // Write the next style index into all the images in the current batch.
  if (next_style_ != style_indices_.end()) {
    for (st_example& example : current_batch_.examples) {
      example.style_index = *next_style_;
    }
    ++next_style_;
  }

  return current_batch_;
}

TrainingProgress ProgressUpdater::Invoke(EncodedBatch batch) {
  auto reduce = [](const shared_float_array& array) {
    float loss = std::accumulate(array.data(), array.data() + array.size(), 0.f,
                                 std::plus<float>());
    loss /= array.size();
    return loss;
  };

  // Compute the loss for this batch.
  float batch_loss = reduce(batch.encoded_data.at("loss"));

  // Update our rolling average (smoothed) loss.
  if (smoothed_loss_) {
    *smoothed_loss_ *= 0.9f;
    *smoothed_loss_ += 0.1f * batch_loss;
  } else {
    // Initialize smoothed loss to the first loss value.
    smoothed_loss_.reset(new float(batch_loss));
  }

  // Write smoothed loss into the result.
  TrainingProgress progress;
  progress.iteration_id = batch.iteration_id;
  progress.smoothed_loss = *smoothed_loss_;

  // Write optional loss components into the result.
  auto style_loss_it = batch.encoded_data.find("style_loss");
  if (style_loss_it != batch.encoded_data.end()) {
    progress.style_loss = reduce(style_loss_it->second);
  }
  auto content_loss_it = batch.encoded_data.find("content_loss");
  if (content_loss_it != batch.encoded_data.end()) {
    progress.content_loss = reduce(content_loss_it->second);
  }

  return progress;
}

// static
float_array_map Checkpoint::ExtractWeights(
    std::unique_ptr<model_spec> nn_spec) {
  float_array_map result = nn_spec->export_params_view();
  for (auto& name_and_weights : result) {
    // The original values will not be valid once the nn_spec is deconstructed.
    // TODO: Ideally this would not require copying. But we should move away
    // from using the protocol buffer as our primary representation anyway.
    name_and_weights.second = shared_float_array::copy(name_and_weights.second);
  }
  return result;
}

std::shared_ptr<Publisher<TrainingProgress>>
ModelTrainer::AsTrainingBatchPublisher(
    std::unique_ptr<data_iterator> training_data,
    const std::string& vgg_mlmodel_path, int offset,
    std::unique_ptr<float> initial_training_loss, compute_context* context) {
  auto iterator = std::make_shared<DataIterator>(std::move(training_data),
                                                 config_.batch_size, offset);

  int height = config_.training_image_height;
  int width = config_.training_image_width;
  auto encode = [height, width](DataBatch batch) {
    return EncodeTrainingBatch(std::move(batch), width, height);
  };

  std::shared_ptr<model_backend> backend =
      CreateTrainingBackend(vgg_mlmodel_path, context);
  auto train = [backend](EncodedBatch batch) {
    EncodedBatch result;
    result.iteration_id = batch.iteration_id;
    result.encoded_data = backend->train(batch.encoded_data);
    return result;
  };

  auto update_progress =
      std::make_shared<ProgressUpdater>(std::move(initial_training_loss));

  return iterator->AsPublisher()->Map(encode)->Map(train)->Map(update_progress);
}

std::shared_ptr<Publisher<DataBatch>> ModelTrainer::AsInferenceBatchPublisher(
    std::unique_ptr<data_iterator> test_data, std::vector<int> style_indices,
    compute_context* context) {
  auto base_iterator = std::make_shared<DataIterator>(
      std::move(test_data), /* batch_size */ 1, /* offset */ 0);
  auto iterator = std::make_shared<InferenceDataIterator>(
      base_iterator, std::move(style_indices));

  std::shared_ptr<model_backend> backend = CreateInferenceBackend(context);
  auto predict = [backend](EncodedInferenceBatch batch) {
    EncodedInferenceBatch result;
    result.iteration_id = batch.iteration_id;
    result.encoded_data = backend->predict(batch.encoded_data);
    result.style_index = batch.style_index;
    return result;
  };

  return iterator->AsPublisher()
      ->Map(EncodeInferenceBatch)
      ->Map(predict)
      ->Map(DecodeInferenceBatch);
}

EncodedBatch EncodeTrainingBatch(DataBatch batch, int width, int height) {
  EncodedBatch result;
  result.iteration_id = batch.iteration_id;

  result.encoded_data = prepare_batch(batch.examples, width, height,
                                      /* train */ true);

  return result;
}

EncodedInferenceBatch EncodeInferenceBatch(DataBatch batch) {
  EncodedInferenceBatch result;
  result.iteration_id = batch.iteration_id;
  result.encoded_data = prepare_predict(batch.examples.front());
  result.style_index = static_cast<int>(batch.examples.front().style_index);
  return result;
}

DataBatch DecodeInferenceBatch(EncodedInferenceBatch batch) {
  DataBatch result;
  result.iteration_id = batch.iteration_id;

  shared_float_array output = batch.encoded_data.at("output");
  std::vector<std::pair<flex_int, flex_image>> processed_batch =
      process_output(output, batch.style_index);
  result.examples.resize(processed_batch.size());
  std::transform(processed_batch.begin(), processed_batch.end(),
                 result.examples.begin(),
                 [](const std::pair<flex_int, flex_image>& style_and_image) {
                   st_example example;
                   example.style_index = style_and_image.first;
                   example.style_image = style_and_image.second;
                   return example;
                 });

  return result;
}

}  // namespace style_transfer
}  // namespace turi
