/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <time.h>
#include <limits.h>
#include <math.h>
#include <iomanip>
#include <logger/assertions.hpp>
#include "data_preparation.hpp"
#include "random/random.hpp"
#include "util/sys_util.hpp"

namespace turi {
namespace sdk_model {
namespace drawing_recognition {

struct Point {
    float x;
    float y;
};

struct Line {
    // ax + by + c = 0
    float slope;
    float a;
    float b;
    float c;
};

void make_point(Point *P, const flex_dict &point_dict) {
    flex_string x ("x");
    flex_string y ("y");
    for (auto key_value_pair: point_dict) {
        const flex_string &key = key_value_pair.first.get<flex_string>();
        const flex_float &value = key_value_pair.second.get<flex_float>();
        if (key.compare(x) == 0) {
            P->x = value;
        } else {
            P->y = value;
        }
    }
}

float line_to_point_distance(Line *L, const flex_dict &point) {
    Point P; make_point(&P, point);
    int x = (int)(P.x);
    int y = (int)(P.y);
    float a = L->a; float b = L->b; float c = L->c;
    float distance = (float)(fabs(a*x+b*y+c))/sqrt(a*a+b*b);
    return (int)distance;
}

void initialize_line(Line *L, const flex_dict &p1, const flex_dict &p2) {
    Point P1; make_point(&P1, p1);
    Point P2; make_point(&P2, p2);
    float start_x = P1.x;
    float start_y = P1.y;
    float end_x = P2.x;
    float end_y = P2.y;
    if (start_x == start_y) {
        L->slope = INT_MAX;
    } else {
        L->slope = ((float)(end_y - start_y))/((float)(end_x - start_x));
    }
    L->a = L->slope;
    L->b = -1;
    L->c = start_y - L->slope * start_x;
}

template<typename T>
std::vector<T> slice(std::vector<T> const &v, int m, int n) {
    auto first = v.cbegin() + m;
    auto last = v.cbegin() + n + 1;
    std::vector<T> vec(first, last);
    return vec;
}

flex_list ramer_douglas_peucker(flex_list stroke, float epsilon) {
    float dmax = 0;
    int index = 0;
    flex_list compressed_stroke;
    int num_points = stroke.size();
    Line L;
    const flex_dict &first_point = stroke[0].get<flex_dict>();
    const flex_dict &last_point = stroke[num_points-1].get<flex_dict>();
    initialize_line(&L, first_point, last_point);
    for (int i = 0; i < num_points; i++) {
        float d = line_to_point_distance(&L, stroke[i].get<flex_dict>());
        if (d > dmax) {
            index = i;
            dmax = d;
        }
    }
    if (dmax > epsilon) {
        flex_list first_slice = slice(stroke, 0, index-1);
        flex_list last_slice = slice(stroke, index, num_points-1);
        compressed_stroke = ramer_douglas_peucker(first_slice, epsilon);
        flex_list rec_results_2 = ramer_douglas_peucker(last_slice, epsilon);
        compressed_stroke.insert(compressed_stroke.end(), 
            rec_results_2.begin(), rec_results_2.end());
    } else {
        compressed_stroke.push_back(first_point);
        compressed_stroke.push_back(last_point);
    }
    return compressed_stroke;
}

flex_list simplify_stroke(flex_list raw_drawing) {
    size_t num_strokes = raw_drawing.size();
    size_t min_x = INT_MAX;
    size_t max_x = 0;
    size_t min_y = INT_MAX;
    size_t max_y = 0;
    flex_list simplified_drawing;
    Point P;
    // Compute bounds of the drawing
    for (size_t i = 0; i < num_strokes; i++) {
        const flex_list &stroke = raw_drawing[i].get<flex_list>();
        for (size_t j = 0; j < stroke.size(); j++) {
            const flex_dict &point = stroke[j].get<flex_dict>();
            make_point(&P, point);
            float x = P.x;
            float y = P.y;
            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
        }
    }
    // Align the drawing to top-left corner and scale to [0,255]
    for (size_t i = 0; i < num_strokes; i++) {
        flex_list &stroke = raw_drawing[i].mutable_get<flex_list>();
        flex_list new_stroke;
        for (size_t j = 0; j < stroke.size(); j++) {
            float new_x;
            float new_y;
            flex_dict &point = stroke[j].mutable_get<flex_dict>();
            make_point(&P, point);
            if (max_x == min_x) {
                // vertical straight line
                new_x = (float)min_x;
            } else {
                new_x = ((P.x - min_x) * 255.0) / (max_x - min_x);
            }
            if (max_y == min_y) {
                // horizontal straight line
                new_y = (float)min_y;
            } else {
                new_y = ((P.y - min_y) * 255.0) / (max_y - min_y);
            }
            flex_dict new_point;
            new_point.push_back(std::make_pair("x", new_x));
            new_point.push_back(std::make_pair("y", new_y));
            // for (auto key_value_pair: point) {
            //     const flex_string &key = key_value_pair.first.get<flex_string>();
            //     const flex_float &value = key_value_pair.second.get<flex_float>();
            //     if (key.compare("x") == 0) {
            //         value = new_x;
            //     } else {
            //         value = new_y;
            //     }
            // }
            new_stroke.push_back(new_point);
        }
        // Apply RDP Line Algorithm
        flex_list compressed_stroke = ramer_douglas_peucker(new_stroke, 2.0);
        simplified_drawing.push_back(compressed_stroke);
    }
    // for (size_t i = 0; i < num_strokes; i++) {
    //     flex_list &stroke = raw_drawing[i].mutable_get<flex_list>();
    //     flex_list compressed_stroke = ramer_douglas_peucker(stroke, 2.0);
    // }
    return simplified_drawing;
}

flex_list rasterize(flex_list simplified_drawing) {
    // size_t num_strokes = simplified_drawing.size();
    
    return simplified_drawing;
}

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
    flex_list normalized_drawing = simplify_stroke(stroke_based_drawing);
    flex_list bitmap = rasterize(normalized_drawing);
    return bitmap;
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
