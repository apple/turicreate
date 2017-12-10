/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <time.h>
#include <iomanip>
#include "sequence_iterator.hpp"
#include "random/random.hpp"

namespace turi {
namespace sdk_model {
namespace activity_classification {


static std::map<std::string,size_t> generate_column_index_map(const std::vector<std::string>& column_names) {
    std::map<std::string,size_t> index_map;
    for (size_t k=0; k < column_names.size(); ++k) {
        index_map[column_names[k]] = k;
    }
    return  index_map;
}

/**
 *  Find the statistical mode (majority value) of a given vector.
 *  Based on https://en.wikipedia.org/wiki/Boyer–Moore_majority_vote_algorithm
 *
 * \param[in] input_vec a vector for which the mode will be calculated

 * \return    The most frequent value within the given vector.
 */
static double vec_majority_value(const flex_vec& input_vec) {
    size_t counter = 0;
    double candidate;
    for (size_t i = 0; i < input_vec.size(); ++i) {
        double value = input_vec[i];
        if (counter == 0) {
            candidate = value;
            counter = 1;
        } else if (candidate == value) {
            counter++;
        } else {
            counter--;
        }
    }
    return candidate;
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
            curr_chunk_targets.push_back(vec_majority_value(curr_window_targets));
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
    for (auto &feat : features) {
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

            if (curr_window_targets.size() == prediction_window) {
                auto target_val = vec_majority_value(curr_window_targets);
                curr_chunk_targets.push_back(target_val);
                curr_window_targets.clear();
            }
        }
        // Check if the aggregated chunk data has reached the maximal chunk length, and finalize
        // the chunk processing.
        if (curr_chunk_features.size() == feature_size) {
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

} //activity_classification
} //sdk_model
} //turi
