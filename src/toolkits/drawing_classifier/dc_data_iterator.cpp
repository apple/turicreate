/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <model_server/extensions/additional_sframe_utilities.hpp>
#include <toolkits/drawing_classifier/dc_data_iterator.hpp>

#include <algorithm>
#include <cmath>

#include <core/data/image/io.hpp>
#include <core/logging/logger.hpp>

namespace turi {
namespace drawing_classifier {

using neural_net::shared_float_array;

simple_data_iterator::target_properties
simple_data_iterator::compute_properties(
    const gl_sarray& targets,
    std::vector<std::string> expected_class_labels) {

  target_properties result;

  // Determine the list of unique class labels,
  gl_sarray classes = targets.unique().sort();

  if (expected_class_labels.empty()) {

    // Infer the class-to-index map from the observed labels.
    result.classes.reserve(classes.size());
    int i = 0;
    for (const flexible_type& label : classes.range_iterator()) {
      result.classes.push_back(label);
      result.class_to_index_map[label] = i++;
    }
  } else {

    // Construct the class-to-index map from the expected labels.
    result.classes = std::move(expected_class_labels);
    int i = 0;
    for (const std::string& label : result.classes) {
      result.class_to_index_map[label] = i++;
    }

    // Use the map to verify that we only encountered expected labels.
    for (const flexible_type& ft : classes.range_iterator()) {
      std::string label(ft);  // Ensures correct overload resolution below.
      if (result.class_to_index_map.count(label) == 0) {
        log_and_throw("Targets contained unexpected class label " + label);
      }
    }
  }
  return result;
}

simple_data_iterator::simple_data_iterator(const parameters& params)
  : data_(params.data),

    // Determine which column is which within each (ordered) row.
    target_index_(data_.column_index(params.target_column_name)),
    predictions_index_(params.predictions_column_name.empty()
                       ? -1
                       : data_.column_index(params.predictions_column_name)),
    feature_index_(data_.column_index(params.feature_column_name)),

    // Whether to traverse the SFrame more than once, and whether to shuffle.
    repeat_(params.repeat),
    shuffle_(params.shuffle),

    // Identify/verify the class labels and other target properties.
    target_properties_(
        compute_properties(data_[params.target_column_name],
                           params.class_labels)),

    // Start an iteration through the entire SFrame.
    range_iterator_(data_.range_iterator()),
    next_row_(range_iterator_.begin()),

    // Initialize random number generator.
    random_engine_(params.random_seed)

{}

void add_drawing_pixel_data_to_batch(
    float *next_drawing_pointer, const flex_image &bitmap) {
  image_load_to_numpy(bitmap, (size_t)next_drawing_pointer, 
    {
      bitmap.m_width * bitmap.m_channels * sizeof(float),
      bitmap.m_channels * sizeof(float),
      sizeof(float)
    });
  next_drawing_pointer += (kDrawingHeight * kDrawingWidth * kDrawingChannels);
}


data_iterator::batch simple_data_iterator::next_batch(size_t batch_size) {

  size_t image_data_size = kDrawingHeight * kDrawingWidth * kDrawingChannels;
  std::vector<float> batch_drawings;
  std::vector<float> batch_targets;
  std::vector<float> batch_predictions;
  batch_drawings.reserve(batch_size * image_data_size);
  batch_targets.reserve(batch_size);
  batch_predictions.reserve(batch_size);
  float *next_drawing_pointer = batch_drawings.data();
  while (batch_targets.size() < batch_size
      && next_row_ != range_iterator_.end()) {
    const sframe_rows::row& row = *next_row_;
    float preds = -1;
    if (predictions_index_ >= 0) {
      preds = row[predictions_index_].to<flex_float>();
    }
    add_drawing_pixel_data_to_batch(next_drawing_pointer,
      row[feature_index_].to<flex_image>());
    batch_targets.emplace_back(row[target_index_].to<flex_float>());
    batch_predictions.emplace_back(preds);

    if (++next_row_ == range_iterator_.end() && repeat_) {

      if (shuffle_) {
        // Shuffle the data.
        // TODO: This heavyweight shuffle operation introduces spikes into the
        // wall-clock time of this function. SFrame should either provide an
        // optimized implementation, or we should implement an approach that
        // amortizes the cost across calls.
        gl_sarray indices = gl_sarray::from_sequence(0, data_.size());
        std::uniform_int_distribution<uint64_t> dist(0);  // 0 to max uint64_t
        uint64_t random_mask = dist(random_engine_);
        auto randomize_indices = [random_mask](const flexible_type& x) {
          uint64_t masked_index = random_mask ^ x.to<uint64_t>();
          return flexible_type(hash64(masked_index));
        };
        data_.add_column(indices.apply(randomize_indices,
                                       flex_type_enum::INTEGER,
                                       /* skip_undefined */ false),
                         "_random_order");
        data_ = data_.sort("_random_order");
        data_.remove_column("_random_order");
      }

      // Reset iteration.
      range_iterator_ = data_.range_iterator();
      next_row_ = range_iterator_.begin();
    }
  }

  // Wrap the buffers as float_array values.
  data_iterator::batch result;
  result.drawings = shared_float_array::wrap(
      std::move(batch_drawings),
      { batch_size, kDrawingHeight, kDrawingWidth, kDrawingChannels, 1 });
  result.targets = shared_float_array::wrap(
      std::move(batch_targets), { batch_size, 1 });
  if (batch_predictions[0] != -1) {
    result.predictions = shared_float_array::wrap(
        std::move(batch_predictions), { batch_size, 1 });
  }

  return result;
}



}  // namespace drawing_classifier
}  // namespace turi

