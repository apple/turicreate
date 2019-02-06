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

flex_list convert_stroke_based_drawing_to_bitmap(
    flex_list stroke_based_drawing) {
    // need stuff here
    return stroke_based_drawing;
}

gl_sframe _drawing_recognition_prepare_data(const gl_sframe &data,
                                            const std::string &feature,
                                            const std::string &target,
                                            const bool is_stroke_input) {
    DASSERT_TRUE(data.contains_column(feature));
    DASSERT_TRUE(data.contains_column(target));

    flex_list bitmap_column;
    flex_list label_column;
    auto column_index_map = generate_column_index_map(data.column_names());
    
    if (is_stroke_input) {
        // need to convert strokes to bitmap
        // Prepare an output SFrame writer, that will write a new SFrame in the converted batch-processing
        // ready format.
        std::vector<std::string> output_column_names = {"bitmap", "label"};
        std::vector<flex_type_enum> output_column_types = {
            flex_type_enum::LIST, data[target].dtype()
        };
        
        gl_sframe_writer output_writer(output_column_names, output_column_types, 1);

        time_t last_print_time = time(0);
        size_t processed_lines = 0;

        // Iterate over the user data. The features and targets are aggregated, and handled
        // whenever a the ending of a prediction_window, chunk or session is reached.
        for (const auto& line: data.range_iterator()) {

            
            auto current_label = line[column_index_map[target]];
            flex_list current_stroke_based_drawing = line[column_index_map[feature]];
            flex_list current_bitmap = convert_stroke_based_drawing_to_bitmap(
                current_stroke_based_drawing);
            
            label_column.push_back(current_label);
            bitmap_column.push_back(current_bitmap);

            output_writer.write({current_bitmap, current_label}, 0);

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

        gl_sframe converted_sframe = output_writer.close();
        converted_sframe.materialize();
        return converted_sframe;

    } else {
        // already a bitmap, we're good
        return data;
    }
}    

} //drawing_recognition
} //sdk_model
} //turi
