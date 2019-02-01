/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <time.h>
#include <iomanip>
#include <logger/assertions.hpp>
#include "data_preparation.hpp"
#include "random/random.hpp"
#include "util/sys_util.hpp"

namespace turi {
namespace sdk_model {
namespace drawing_recognition {

static std::map<std::string,size_t> generate_column_index_map(
    const std::vector<std::string>& column_names) {
    std::map<std::string,size_t> index_map;
    for (size_t k=0; k < column_names.size(); ++k) {
        index_map[column_names[k]] = k;
    }
    return  index_map;
}

flex_nd_vec convert_stroke_based_drawing_to_bitmap(
    flex_nd_vec stroke_based_drawing) {
    // need stuff here
    return stroke_based_drawing;
}

gl_sframe _drawing_recognition_prepare_data(const gl_sframe &data,
                                            const std::string &feature,
                                            const std::string &target,
                                            const bool is_stroke_input) {
    DASSERT_TRUE(data.contains_column(feature));
    DASSERT_TRUE(data.contains_column(target));

    flex_vec &bitmap_column;
    flex_vec &label_column;
    auto column_index_map = generate_column_index_map(data.column_names());
    
    if (is_stroke_input) {
        // need to convert strokes to bitmap
        // Prepare an output SFrame writer, that will write a new SFrame in the converted batch-processing
        // ready format.
        std::vector<std::string> output_column_names = {"bitmap", "label"};
        std::vector<flex_type_enum> output_column_types = {
            flex_type_enum::ND_VECTOR, data[target].dtype()
        };
        
        gl_sframe_writer output_writer(output_column_names, output_column_types, 1);

        // if (verbose) {
        //   logprogress_stream << "Using sequences of size " << chunk_size << " for model creation." << std::endl;
        // }

        time_t last_print_time = time(0);
        size_t processed_lines = 0;

        // Iterate over the user data. The features and targets are aggregated, and handled
        // whenever a the ending of a prediction_window, chunk or session is reached.
        for (const auto& line: data.range_iterator()) {

            auto current_label = line[column_index_map[target]];
            auto current_stroke_based_drawing = line[column_index_map[feature]];
            auto current_bitmap = convert_stroke_based_drawing_to_bitmap(
                current_stroke_based_drawing);

            label_column.push_back(current_label); 
            // argument needs to be a flex_vec
            bitmap_column.push_back(current_bitmap);
            // argument needs to be a flex_vec

            // If target column exists, the targets are aggregated for the duration
            // of prediction_window.
            // Each prediction_window subsampled into a single target value, by selecting
            // the most frequent value (statistical mode) within the window.
            // if (use_target) {
            //     curr_window_targets.push_back(line[column_index_map[target]]);

            //     if (curr_window_targets.size() == static_cast<size_t>(prediction_window)) {
            //         auto target_val = vec_mode(curr_window_targets);
            //         curr_chunk_targets.push_back(target_val);
            //         curr_window_targets.clear();
            //     }
            // }
            // Check if the aggregated chunk data has reached the maximal chunk length, and finalize
            // the chunk processing.
            // if (curr_chunk_features.size() == static_cast<size_t>(feature_size)) {
            //     finalize_chunk(curr_chunk_features,
            //                    curr_chunk_targets,
            //                    curr_window_targets,
            //                    curr_session_id,
            //                    output_writer,
            //                    chunk_size,
            //                    feature_size,
            //                    predictions_in_chunk,
            //                    use_target);
            // }

            time_t now = time(0);
            bool verbose = false; 
            // maybe have two wrappers around this function: 
            // verbose and non-verbose
            if (verbose && difftime(now, last_print_time) > 10) {
                logprogress_stream << "Pre-processing: " << std::setw(3) << (100 * processed_lines / data.size()) << "% complete"  << std::endl;
                last_print_time = now;
            }

            processed_lines += 1;
        }

        output_writer.write({bitmap_column, label_column}, 0);

        // Handle the tail of the data - the last few lines of the last chunk, which needs to be finalized.
        // if (curr_chunk_features.size() > 0) {
        //     finalize_chunk(curr_chunk_features,
        //                    curr_chunk_targets,
        //                    curr_window_targets,
        //                    last_session_id,
        //                    output_writer,
        //                    chunk_size,
        //                    feature_size,
        //                    predictions_in_chunk,
        //                    use_target);
        // }

        // Update the count of the last session in the dataset
        // number_of_sessions++;

        gl_sframe converted_sframe = output_writer.close();
        converted_sframe.materialize();

    } else {
        // already a bitmap, we're good
        return data;
    }
}    

} //drawing_recognition
} //sdk_model
} //turi
