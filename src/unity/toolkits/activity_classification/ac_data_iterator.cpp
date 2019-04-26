/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/activity_classification/ac_data_iterator.hpp>

#include <time.h>

#include <algorithm>
#include <iomanip>
#include <sstream>

#include <logger/assertions.hpp>
#include <logger/logger.hpp>
#include <random/random.hpp>
#include <util/sys_util.hpp>

namespace turi {
namespace activity_classification {

namespace {

using ::turi::neural_net::shared_float_array;

static std::map<std::string,size_t> generate_column_index_map(const std::vector<std::string>& column_names) {
    std::map<std::string,size_t> index_map;
    for (size_t k=0; k < column_names.size(); ++k) {
        index_map[column_names[k]] = k;
    }
    return  index_map;
}

/**
 *  Find the statistical mode (majority value) of a given vector.
 *
 * \param[in] input_vec a vector for which the mode will be calculated

 * \return    The most frequent value within the given vector.
 */

static double vec_mode(const flex_vec& input_vec) {
    std::vector<int> histogram;
    for (size_t i = 0; i < input_vec.size(); ++i) {
        size_t value = static_cast<size_t>(input_vec[i]);

        // Each value should be in the index of a class label
        DASSERT_EQ(static_cast<double>(static_cast<size_t>(input_vec[i])), input_vec[i]);

        if(histogram.size() < (value + 1)){
          histogram.resize(value + 1);
        }

        histogram[value]++;
    }

    auto majority = std::max_element(histogram.begin(), histogram.end());
    
    // return index to mode majority value
    return std::distance(histogram.begin(), majority);
}

/**
 * Write the aggregated data of the current chunk as a single new vector in the converted SFrame,
 * and init all aggregation vectors to begin a new chunk.
 *
 * \param[in,out]   curr_chunk_features     A vector with the aggregated features data (flattened row-major) of the current chunk.
 * \param[in,out]   curr_chunk_targets      A vector with the aggregated target data (flattened row-major) of the current chunk - after subsampling by predicion_window.
 * \param[in,out]   curr_window_targets     A vector with the aggregate raw target data (flattened row-major) of the current precdiction window, in case it was not finalized yet.
 * \param[in,out]   output_writer           A gl_sframe_writer object, which is used to write the converted SFrame.
 * \param[in]       curr_session_id         The current session id - may be integer or string.
 * \param[in]       chunk_size              The constant sequence length which is used for training and predicion.
 * \param[in]       predictions_in_chunk    The numer of prediction windows in each chunks. This is also the number of target values per chunk.
 * \param[in]       use_target              A bool indicating whether the user's dataset includes a target column.
 */
static void finalize_chunk(flex_vec&            curr_chunk_features,
                           flex_vec&            curr_chunk_targets,
                           flex_vec&            curr_window_targets,
                           flexible_type        curr_session_id,
                           gl_sframe_writer&    output_writer,
                           size_t               chunk_size,
                           size_t               feature_size,
                           size_t               predictions_in_chunk,
                           bool                 use_target) {

    size_t curr_feature_size = curr_chunk_features.size();
    size_t num_features = feature_size / chunk_size;
    size_t curr_chunk_size = curr_feature_size / num_features;

    // If the required chunk length hasn't been reached (may happen in the last chunk
    // of each session) - the features vector needs to be padded.
    // The vector is padded to feature_size
    if (curr_feature_size < feature_size) {
        curr_chunk_features.resize(feature_size, 0.0);
    }

    if (use_target) {
        if (curr_window_targets.size() > 0) {
            curr_chunk_targets.push_back(vec_mode(curr_window_targets));
            curr_window_targets.clear();
        }

        // The last chunk of each session may be padded, in which case the target data should be ignored.
        // This is achieved by setting a weight of "1" for all actual targets, and "0" for padded data.
        flex_vec curr_chunk_weights(curr_chunk_targets.size(), 1.0);

        if (curr_chunk_targets.size() < predictions_in_chunk) {
            curr_chunk_targets.resize(predictions_in_chunk, 0.0);
            curr_chunk_weights.resize(predictions_in_chunk, 0.0);
        }

        output_writer.write({curr_chunk_features,
                             curr_chunk_size,
                             curr_session_id,
                             curr_chunk_targets,
                             curr_chunk_weights}, 0);
    } else {
        output_writer.write({curr_chunk_features, curr_chunk_size, curr_session_id}, 0);
    }

    curr_chunk_features.clear();
    curr_chunk_targets.clear();

    return;
}

variant_map_type _activity_classifier_prepare_data_impl(const gl_sframe &data,
                                                        const std::vector<std::string> &features,
                                                        const std::string &session_id,
                                                        const int &prediction_window,
                                                        const int &predictions_in_chunk,
                                                        const std::string &target,
                                                        bool verbose) {

    DASSERT_TRUE(features.size() > 0);
    DASSERT_TRUE(prediction_window > 0);
    DASSERT_TRUE(predictions_in_chunk > 0);
    DASSERT_TRUE(data.contains_column(session_id));
    for (TURI_ATTRIBUTE_UNUSED_NDEBUG auto &feat : features) {
        DASSERT_TRUE(data.contains_column(feat));
    }

    bool use_target = (target != "");
    if (use_target) {
        DASSERT_TRUE(data.contains_column(target));
    }

    if (verbose) {
        logprogress_stream << "Pre-processing " << data.size() << " samples..." << std::endl;
    }

    int chunk_size = prediction_window * predictions_in_chunk;
    int feature_size = chunk_size * features.size();

    // Build a dict pf the column order by column name, to later access within the iterator
    auto column_index_map = generate_column_index_map(data.column_names());
    
    flex_vec curr_chunk_targets;
    flex_vec curr_chunk_features;
    curr_chunk_features.reserve(feature_size);
    flex_vec curr_window_targets;
    flexible_type last_session_id = data[session_id][0];

    size_t number_of_sessions = 0;

    // Prepare an output SFrame writer, that will write a new SFrame in the converted batch-processing
    // ready format.
    std::vector<std::string> output_column_names = {"features", "chunk_len", "session_id"};
    std::vector<flex_type_enum> output_column_types = {flex_type_enum::VECTOR, flex_type_enum::INTEGER,
                                                        data[session_id].dtype()};
    if (use_target) {
        curr_chunk_targets.reserve(predictions_in_chunk);
        curr_window_targets.reserve(prediction_window);
        output_column_names.insert(output_column_names.end(), {"target", "weights"});
        output_column_types.insert(output_column_types.end(), {flex_type_enum::VECTOR, flex_type_enum::VECTOR});
    }
    gl_sframe_writer output_writer(output_column_names, output_column_types, 1);

    if (verbose) {
      logprogress_stream << "Using sequences of size " << chunk_size << " for model creation." << std::endl;
    }

    time_t last_print_time = time(0);
    size_t processed_lines = 0;

    // Iterate over the user data. The features and targets are aggregated, and handled
    // whenever a the ending of a prediction_window, chunk or session is reached.
    for (const auto& line: data.range_iterator()) {

        auto curr_session_id = line[column_index_map[session_id]];

        // Check if a new session has started
        if (curr_session_id != last_session_id) {

            // Finalize the last chunk of the previous session
            if (curr_chunk_features.size() > 0) {
                finalize_chunk(curr_chunk_features,
                               curr_chunk_targets,
                               curr_window_targets,
                               last_session_id,
                               output_writer,
                               chunk_size,
                               feature_size,
                               predictions_in_chunk,
                               use_target);
            }

            last_session_id = curr_session_id;
            number_of_sessions++;
        }

        for (const auto feature_name : features) {
            curr_chunk_features.push_back(line[column_index_map[feature_name]]);
        }

        // If target column exists, the targets are aggregated for the duration
        // of prediction_window.
        // Each prediction_window subsampled into a single target value, by selecting
        // the most frequent value (statistical mode) within the window.
        if (use_target) {
            curr_window_targets.push_back(line[column_index_map[target]]);

            if (curr_window_targets.size() == static_cast<size_t>(prediction_window)) {
                auto target_val = vec_mode(curr_window_targets);
                curr_chunk_targets.push_back(target_val);
                curr_window_targets.clear();
            }
        }
        // Check if the aggregated chunk data has reached the maximal chunk length, and finalize
        // the chunk processing.
        if (curr_chunk_features.size() == static_cast<size_t>(feature_size)) {
            finalize_chunk(curr_chunk_features,
                           curr_chunk_targets,
                           curr_window_targets,
                           curr_session_id,
                           output_writer,
                           chunk_size,
                           feature_size,
                           predictions_in_chunk,
                           use_target);
        }

        time_t now = time(0);
        if (verbose && difftime(now, last_print_time) > 10) {
            logprogress_stream << "Pre-processing: " << std::setw(3) << (100 * processed_lines / data.size()) << "% complete"  << std::endl;
            last_print_time = now;
        }

        processed_lines += 1;
    }

    // Handle the tail of the data - the last few lines of the last chunk, which needs to be finalized.
    if (curr_chunk_features.size() > 0) {
        finalize_chunk(curr_chunk_features,
                       curr_chunk_targets,
                       curr_window_targets,
                       last_session_id,
                       output_writer,
                       chunk_size,
                       feature_size,
                       predictions_in_chunk,
                       use_target);
    }

    // Update the count of the last session in the dataset
    number_of_sessions++;


    if (verbose) {
        logprogress_stream << "Processed a total of " << number_of_sessions << " sessions." << std::endl;
    }
    gl_sframe converted_sframe = output_writer.close();
    converted_sframe.materialize();

    variant_map_type result_dict;
    result_dict["converted_data"] = converted_sframe;
    result_dict["num_of_sessions"] = number_of_sessions;

    return result_dict;
}

}  // namespace

variant_map_type _activity_classifier_prepare_data( const gl_sframe &data,
                                                    const std::vector<std::string> &features,
                                                    const std::string &session_id,
                                                    const int &prediction_window,
                                                    const int &predictions_in_chunk,
                                                    const std::string &target) {
  return _activity_classifier_prepare_data_impl(data, features, session_id,
      prediction_window, predictions_in_chunk, target, false);
}

variant_map_type _activity_classifier_prepare_data_verbose( const gl_sframe &data,
                                                            const std::vector<std::string> &features,
                                                            const std::string &session_id,
                                                            const int &prediction_window,
                                                            const int &predictions_in_chunk,
                                                            const std::string &target) {
  return _activity_classifier_prepare_data_impl(data, features, session_id,
      prediction_window, predictions_in_chunk, target, true);
}

data_iterator::~data_iterator() = default;

// static
simple_data_iterator::preprocessed_data simple_data_iterator::preprocess_data(
    const parameters& params) {

  gl_sframe data = params.data;
  flex_list class_labels = params.class_labels;
  bool has_target = !params.target_column_name.empty();

  std::vector<std::string> feature_column_names = params.feature_column_names;
  if (feature_column_names.empty()) {
    // Default to using all columns besides the target and session-id columns.
    feature_column_names = data.column_names();
    auto new_end = std::remove(feature_column_names.begin(),
                               feature_column_names.end(),
                               params.target_column_name);
    new_end = std::remove(feature_column_names.begin(), new_end,
                          params.session_id_column_name);
    feature_column_names.erase(new_end, feature_column_names.end());
  }

  if (has_target) {

    // Copy the SFrame so we can mutate it without affecting the caller's copy.
    data = data.select_columns(data.column_names());

    // Assemble the list of class labels if necessary.
    if (class_labels.empty()) {
      gl_sarray target_values = data[params.target_column_name].unique().sort();
      gl_sarray_range range = target_values.range_iterator();
      class_labels = flex_list(range.begin(), range.end());
    }

    // Replace the target column with an encoded version.
    auto encoding_fn = [&class_labels](const flexible_type& ft) {
      auto it = std::find(class_labels.begin(), class_labels.end(), ft);
      if (it == class_labels.end()) {
        std::stringstream ss;
        ss << "Cannot evaluate data with unexpected class label " << ft;
        log_and_throw(ss.str());
      }
      return static_cast<float>(it - class_labels.begin());
    };
    data[params.target_column_name] =
      data[params.target_column_name].apply(encoding_fn, flex_type_enum::FLOAT);
  }

  // Chunk the data, so that each row of the resulting SFrame corresponds to a
  // sequence of up to predictions_in_chunk prediction windows (from the same
  // session), each comprising up to prediction_window rows from the original
  // SFrame.
  variant_map_type result_map = _activity_classifier_prepare_data_impl(
      data, feature_column_names, params.session_id_column_name,
      static_cast<int>(params.prediction_window),
      static_cast<int>(params.predictions_in_chunk), params.target_column_name,
      /* verbose */ params.verbose);

  preprocessed_data result;
  result.chunks = variant_get_value<gl_sframe>(result_map.at("converted_data"));
  result.num_sessions =
      variant_get_value<size_t>(result_map.at("num_of_sessions"));
  result.session_id_type = data[params.session_id_column_name].dtype();
  result.has_target = has_target;
  result.feature_names = flex_list(feature_column_names.begin(),
                                   feature_column_names.end());
  result.class_labels = std::move(class_labels);
  return result;
}

simple_data_iterator::simple_data_iterator(const parameters& params)
  : data_(preprocess_data(params)),
    num_samples_per_prediction_(params.prediction_window),
    num_predictions_per_chunk_(params.predictions_in_chunk),
    range_iterator_(data_.chunks.range_iterator()),
    next_row_(range_iterator_.begin()),
    end_of_rows_(range_iterator_.end())
{}

const flex_list& simple_data_iterator::feature_names() const {
  return data_.feature_names;
}

const flex_list& simple_data_iterator::class_labels() const {
  return data_.class_labels;
}

flex_type_enum simple_data_iterator::session_id_type() const {
  return data_.session_id_type;
}

bool simple_data_iterator::has_next_batch() const {
  return next_row_ != end_of_rows_;
}

data_iterator::batch simple_data_iterator::next_batch(size_t batch_size) {

  size_t num_samples_per_chunk =
      num_samples_per_prediction_ * num_predictions_per_chunk_;
  size_t num_features = data_.feature_names.size();
  size_t features_stride = num_samples_per_chunk * num_features;
  size_t features_size = batch_size * features_stride;

  // Identify column indices for future reference.
  size_t features_column_index = data_.chunks.column_index("features");
  size_t chunk_len_column_index = data_.chunks.column_index("chunk_len");
  size_t session_id_column_index = data_.chunks.column_index("session_id");
  size_t labels_column_index = 0;
  size_t weights_column_index = 0;
  if (data_.has_target) {
    labels_column_index = data_.chunks.column_index("target");
    weights_column_index = data_.chunks.column_index("weights");
  }

  // Allocate buffers for the resulting batch data.
  std::vector<float> features(features_size, 0.f);
  std::vector<float> labels;
  std::vector<float> weights;
  if (data_.has_target) {
    size_t labels_size = batch_size * num_predictions_per_chunk_;
    labels.resize(labels_size, 0.f);
    weights.resize(labels_size, 0.f);
  }

  // Iterate through SFrame rows until filling the batch or reaching the end of
  // the data.
  float* features_out = features.data();
  float* labels_out = labels.data();
  float* weights_out = weights.data();
  std::vector<batch::chunk_info> batch_info;
  batch_info.reserve(batch_size);
  while (batch_info.size() < batch_size && next_row_ != end_of_rows_) {

    const sframe_rows::row& row = *next_row_;

    // Copy the feature values (converting from double to float).
    const flex_vec& feature_vec = row[features_column_index].get<flex_vec>();
    ASSERT_EQ(feature_vec.size(), features_stride);
    features_out = std::copy(feature_vec.begin(), feature_vec.end(),
                             features_out);

    if (data_.has_target) {

      // Also copy the labels and weights.
      const flex_vec& target_vec = row[labels_column_index].get<flex_vec>();
      const flex_vec& weight_vec = row[weights_column_index].get<flex_vec>();
      labels_out = std::copy(target_vec.begin(), target_vec.end(), labels_out);
      weights_out = std::copy(weight_vec.begin(), weight_vec.end(),
                              weights_out);
    }

    batch_info.emplace_back();
    batch_info.back().session_id = row[session_id_column_index];
    batch_info.back().num_samples = row[chunk_len_column_index];

    ++next_row_;
  }

  // Wrap the buffers as float_array values.
  batch result;
  result.features = shared_float_array::wrap(
      std::move(features),
      { batch_size, 1, num_samples_per_chunk, num_features });
  if (data_.has_target) {
    result.labels = shared_float_array::wrap(
        std::move(labels), { batch_size, 1, num_predictions_per_chunk_, 1 });
    result.weights = shared_float_array::wrap(
        std::move(weights), { batch_size, 1, num_predictions_per_chunk_, 1 });
  }
  result.batch_info = std::move(batch_info);

  return result;
}

void simple_data_iterator::reset() {

  range_iterator_ = data_.chunks.range_iterator();
  next_row_ = range_iterator_.begin();
  end_of_rows_ = range_iterator_.end();

  // TODO: If gl_sframe_range::end() were a const method, we wouldn't need to
  // store end_of_rows_ separately.
}

}  //activity_classification
}  //turi
