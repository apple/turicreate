/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <algorithm>
#include <iterator>
#include <limits.h>
#include <math.h>
#include <logger/assertions.hpp>
#include <image/image_type.hpp>
#include <util/sys_util.hpp>
#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#endif // __APPLE__

#include "data_preparation.hpp"

namespace turi {
namespace drawing_classifier {

static constexpr size_t INTERMEDIATE_BITMAP_WIDTH = 256;
static constexpr size_t INTERMEDIATE_BITMAP_HEIGHT = 256;
static constexpr size_t FINAL_BITMAP_WIDTH = 28;
static constexpr size_t FINAL_BITMAP_HEIGHT = 28;
static constexpr float STROKE_WIDTH = 20.0f;

namespace {

class Point {
public:
    Point(float x_provided, float y_provided) {
        m_x = x_provided;
        m_y = y_provided;
    }

    Point(const flex_dict &point_dict, 
        int drawing_number = 0, 
        int stroke_index = 0, 
        int point_in_stroke_index = 0) {
        bool found_x = false;
        bool found_y = false;
        for (const auto& key_value_pair: point_dict) {
            const flex_string &key = key_value_pair.first.get<flex_string>();
            flex_float value;
            if (key_value_pair.second.get_type() == flex_type_enum::INTEGER) {
                value = key_value_pair.second.to<flex_float>();
            } else if (key_value_pair.second.get_type() == flex_type_enum::FLOAT) {
                value = key_value_pair.second.get<flex_float>();
            } else {
                log_and_throw("In the drawing in row " + 
                    std::to_string(drawing_number) + " the point at index " 
                    + std::to_string(point_in_stroke_index) + " in the " 
                    + std::to_string(stroke_index) + "th stroke" 
                    + " does not have an appropriate type for the " + key 
                    + " coordinate. Please make sure both the \"x\" and \"y\""
                    + " coordinates are either integers or" 
                    + " floating point numbers.");
            }
            if (key == "x") {
                found_x = true;
                m_x = value;
            } else if (key == "y") {
                found_y = true;
                m_y = value;
            } else {
                // some other unnecessary keys are also present, 
                // but we only need x and y so we will ignore the 
                // other keys
            }
        }

        auto error_message = [drawing_number,
            point_in_stroke_index,
            stroke_index](const char *x_or_y) {
            return ("In the drawing in row " + std::to_string(drawing_number) + 
                " the point at index " + std::to_string(point_in_stroke_index)+ 
                " in the " + std::to_string(stroke_index) + 
                "th stroke does not contain a " + x_or_y + 
                " coordinate. Please make sure the dictionary " + 
                " representing a point has both \"x\" and \"y\" keys.");
        };

        if (!found_x) {
            log_and_throw(error_message("x"));
        } else if (!found_y) {
            log_and_throw(error_message("y"));
        }
    }

    float get_x() {
        return m_x;
    }

    float get_y() {
        return m_y;
    }
private:
    float m_x = 0.0f;
    float m_y = 0.0f;
};

class Line {
public:
    Line(const flex_dict &p1, const flex_dict &p2) {
        Point P1(p1);
        Point P2(p2);
        float start_x = P1.get_x();
        float start_y = P1.get_y();
        float end_x = P2.get_x();
        float end_y = P2.get_y();
        if (start_x == end_x) {
            m_a = std::numeric_limits<float>::max();
        } else {
            m_a = (end_y - start_y)/(end_x - start_x);
        }
        m_b = -1;
        m_c = start_y - m_a * start_x;
    }

    float distance_to_point(const flex_dict &point) {
        Point P(point);
        float x = P.get_x();
        float y = P.get_y();
        float distance = (fabs(m_a * x + m_b * y + m_c))/sqrtf(
            m_a * m_a + m_b * m_b);
        return floorf(distance);
    }
private:
    // ax + by + c = 0
    float m_a;
    float m_b;
    float m_c;
};

} // namespace

/* Ramer Douglas Peucker Algorithm:
 * https://en.wikipedia.org/wiki/Ramer–Douglas–Peucker_algorithm
 */
flex_list ramer_douglas_peucker(
    flex_list::iterator begin, 
    flex_list::iterator end, 
    float epsilon) {
    float dmax = 0;
    flex_list compressed_stroke;
    if (begin == end) {
        return compressed_stroke;
    }
    const flex_dict &first_point = begin->get<flex_dict>();
    const flex_dict &last_point = (end-1)->get<flex_dict>();
    Line L(first_point, last_point);
    flex_list::iterator index_iterator;
    for (auto it = begin; it != end; it++) {
        float d = L.distance_to_point(it->get<flex_dict>());
        if (d > dmax) {
            index_iterator = it;
            dmax = d;
        }
    }
    if (dmax > epsilon) {
        compressed_stroke = ramer_douglas_peucker(
            begin, index_iterator, epsilon);
        flex_list rec_results_2 = ramer_douglas_peucker(
            index_iterator, end, epsilon);
        compressed_stroke.insert(compressed_stroke.end(), 
            rec_results_2.begin(), rec_results_2.end());
    } else {
        compressed_stroke.push_back(first_point);
        compressed_stroke.push_back(last_point);
    }
    return compressed_stroke;
}

flex_list simplify_drawing(flex_list raw_drawing, int row_number) {
    size_t num_strokes = raw_drawing.size();
    float min_x = std::numeric_limits<float>::max();
    float max_x = 0;
    float min_y = std::numeric_limits<float>::max();
    float max_y = 0;
    flex_list simplified_drawing;
    // Compute bounds of the drawing
    for (size_t i = 0; i < num_strokes; i++) {
        const flex_list &stroke = raw_drawing[i].get<flex_list>();
        for (size_t j = 0; j < stroke.size(); j++) {
            const flex_dict &point = stroke[j].get<flex_dict>();
            Point P(point, row_number, i, j);
            float x = P.get_x();
            float y = P.get_y();
            min_x = std::min(x, min_x);
            max_x = std::max(x, max_x);
            min_y = std::min(y, min_y);
            max_y = std::max(y, max_y);
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
            Point P(point);
            if (max_x == min_x) {
                // vertical straight line
                new_x = min_x;
            } else {
                new_x = ((P.get_x() - min_x) * 255.0) / (max_x - min_x);
            }
            if (max_y == min_y) {
                // horizontal straight line
                new_y = min_y;
            } else {
                new_y = ((P.get_y() - min_y) * 255.0) / (max_y - min_y);
            }
            flex_dict new_point;
            new_point.push_back(std::make_pair("x", new_x));
            new_point.push_back(std::make_pair("y", new_y));
            new_stroke.push_back(new_point);
        }
        // Apply RDP Line Algorithm
        if (stroke.size() > 0) {
            simplified_drawing.push_back(ramer_douglas_peucker( 
                new_stroke.begin(),
                new_stroke.end(),
                2.0));
        }
    }
    return simplified_drawing;
}

bool in_bounds(int x, int y, int dim) {
    return (x >= 0 && x < dim && y >= 0 && y < dim);
}

void paint_point(flex_nd_vec &bitmap, int x, int y, int pad) {
    size_t dimension = bitmap.shape()[1];
    for (int dx = -pad; dx < pad; dx++) {
        for (int dy = -pad; dy < pad; dy++) {
            if (in_bounds(x+dx, y+dy, dimension)) {
                bitmap[(y+dy) * dimension + (x+dx)] = 1.0;
            }
        }
    }
}

flex_nd_vec paint_stroke(
    flex_nd_vec bitmap, Point start, Point end, float stroke_width) {
    bool along_x;
    float slope;
    if (floorf(end.get_x()) == floorf(start.get_x())) {
        slope = std::numeric_limits<float>::max();
    } else {
        slope = (end.get_y() - start.get_y())/(end.get_x() - start.get_x());
    }
    int pad = (int)(stroke_width/2);
    along_x = (fabs(slope) < 1);
    if ((along_x && (start.get_x() > end.get_x())) 
        || (!along_x && (start.get_y() > end.get_y()))) {
        std::swap(start, end);
    }
    int x1 = (int)(start.get_x());
    int y1 = (int)(start.get_y());
    int x2 = (int)(end.get_x());
    int y2 = (int)(end.get_y());
    if (along_x) {
        for (int x = x1; x <= x2; x++) {
            int y = (int)(slope * (x - x1) + y1);
            paint_point(bitmap, x, y, pad);
        }
    } else {
        for (int y = y1; y <= y2; y++) {
            int x = (int)(x1 + ((y - y1) / slope));
            paint_point(bitmap, x, y, pad);
        }
    }
    return bitmap;
}

flex_image blur_bitmap(flex_nd_vec bitmap, int ksize) {
    std::vector<size_t> bitmap_shape = bitmap.shape();
    flex_nd_vec blurred_bitmap(bitmap_shape, 0.0);
    int dimension = bitmap_shape[1];
    int pad = ksize/2;
    for (int row = 0; row < dimension; row++) {
        for (int col = 0; col < dimension; col++) {
            int index = row * dimension + col;
            if (row < pad 
                || row >= dimension-pad 
                || col < pad 
                || col >= dimension-pad) {
                blurred_bitmap[index] = std::min(255.0, 255 * bitmap[index]);
                continue;
            }
            double sum_for_blur = 0.0;
            int num_values_in_sum = 0;
            for (int dr = -pad; dr <= pad; dr++) {
                for (int dc = -pad; dc <= pad; dc++) {
                    sum_for_blur += bitmap[(row+dr) * dimension + (col+dc)];
                    num_values_in_sum += 1;
                }
            }
            blurred_bitmap[index] = std::min(255.0, 
                255 * sum_for_blur / num_values_in_sum);
        }
    }
    uint8_t image_data[dimension * dimension];
    for (int idx=0; idx < dimension * dimension; idx++) {
        image_data[idx] = ((uint8_t)(blurred_bitmap[idx]));
    }
    return flex_image((const char *)image_data,
        dimension,                  // height
        dimension,                  // width
        1,                          // channels
        dimension*dimension,        // image_data_size
        IMAGE_TYPE_CURRENT_VERSION, // version
        2);                         // format
        // last argument is turi::Format::RAW_ARRAY, which has a value of 2
}

#ifdef __APPLE__
flex_image rasterize_on_mac(flex_list simplified_drawing) {
    size_t num_strokes = simplified_drawing.size();
    int MAC_OS_STRIDE = 64;
    CGColorSpaceRef grayscale = CGColorSpaceCreateDeviceGray();
    CGContextRef intermediate_bitmap_context = CGBitmapContextCreate(
        NULL, INTERMEDIATE_BITMAP_WIDTH, INTERMEDIATE_BITMAP_HEIGHT, 
        8, 0, grayscale, kCGImageAlphaNone);
    CGContextSetRGBStrokeColor(intermediate_bitmap_context, 1.0, 1.0, 1.0, 1.0);
    CGAffineTransform transform = CGAffineTransformIdentity;
    CGMutablePathRef path = CGPathCreateMutable();
    for (size_t i = 0; i < num_strokes; i++) {
        const flex_list &stroke = simplified_drawing[i].get<flex_list>();
        const flex_dict &start_point_dict = stroke[0].get<flex_dict>();
        size_t num_points_in_stroke = stroke.size();
        Point P(start_point_dict);
        // @TODO: When we make a dylib, we'll need some sophisticated 
        // iOS/macOS checking here to figure out whether we need to 
        // subtract 256 or not........
        CGPathMoveToPoint(path, &transform, 
            P.get_x(), ((float)(INTERMEDIATE_BITMAP_WIDTH))-P.get_y());
        for (size_t j = 1; j < num_points_in_stroke; j++) {
            const flex_dict &point_dict = stroke[j].get<flex_dict>();
            Point P(point_dict);
            CGPathAddLineToPoint(path, &transform, 
                P.get_x(), ((float)(INTERMEDIATE_BITMAP_WIDTH))-P.get_y());
        }
    }
    CGContextSetLineWidth(intermediate_bitmap_context, STROKE_WIDTH);
    CGContextBeginPath(intermediate_bitmap_context);
    CGContextAddPath(intermediate_bitmap_context, path);
    CGContextStrokePath(intermediate_bitmap_context);
    CGImageRef intermediate_bitmap_cg = CGBitmapContextCreateImage(
        intermediate_bitmap_context);
    CGContextRef final_bitmap_context = CGBitmapContextCreate(NULL, 
        FINAL_BITMAP_WIDTH, FINAL_BITMAP_HEIGHT, 8, 0, grayscale, 
        kCGImageAlphaNone);
    CGRect final_bitmap_rect = CGRectMake(0.0, 0.0, 
        ((float)(FINAL_BITMAP_WIDTH)), ((float)(FINAL_BITMAP_HEIGHT)));
    CGContextDrawImage(final_bitmap_context, final_bitmap_rect, 
        intermediate_bitmap_cg);
    CGImageRef final_bitmap_cg = CGBitmapContextCreateImage(
        final_bitmap_context);
    CFDataRef pixel_data = CGDataProviderCopyData(
        CGImageGetDataProvider(final_bitmap_cg));
    const uint8_t* data_ptr = CFDataGetBytePtr(pixel_data);
    uint8_t real_data[FINAL_BITMAP_WIDTH * FINAL_BITMAP_HEIGHT];
    for (size_t row = 0; row < FINAL_BITMAP_WIDTH; row++) {
        for (size_t col = 0; col < FINAL_BITMAP_HEIGHT; col++) {
            // check platform here
            real_data[row * FINAL_BITMAP_WIDTH + col] = (uint8_t)(
                data_ptr[row * MAC_OS_STRIDE + col]);
        }
    }
    CGPathRelease(path);
    CGImageRelease(final_bitmap_cg);
    CGImageRelease(intermediate_bitmap_cg);
    CGContextRelease(final_bitmap_context);
    CGContextRelease(intermediate_bitmap_context);
    CGColorSpaceRelease(grayscale);
    return flex_image((const char *)real_data,      // image_data
        FINAL_BITMAP_HEIGHT,                        // height
        FINAL_BITMAP_WIDTH,                         // width
        1,                                          // channels
        FINAL_BITMAP_WIDTH * FINAL_BITMAP_HEIGHT,   // image_data_size
        IMAGE_TYPE_CURRENT_VERSION,                 // version
        2);                                         // format
        // last argument is turi::Format::RAW_ARRAY, which has a value of 2
}
#endif // __APPLE__

flex_image rasterize(flex_list simplified_drawing) {
    flex_image final_bitmap; // 1 x 28 x 28
    size_t num_strokes = simplified_drawing.size();
#ifdef __APPLE__
    return rasterize_on_mac(simplified_drawing);
#endif // __APPLE__
    std::vector<size_t> intermediate_bitmap_shape {1, INTERMEDIATE_BITMAP_WIDTH, INTERMEDIATE_BITMAP_HEIGHT};
    flex_nd_vec intermediate_bitmap(intermediate_bitmap_shape, 0.0);
    for (size_t i = 0; i < num_strokes; i++) {
        const flex_list &stroke = simplified_drawing[i].get<flex_list>();
        const flex_dict &start_point_dict = stroke[0].get<flex_dict>();
        size_t num_points_in_stroke = stroke.size();
        Point last_point(start_point_dict);
        for (size_t j = 1; j < num_points_in_stroke; j++) {
            const flex_dict &next_point_dict = stroke[j].get<flex_dict>();
            Point next(next_point_dict);
            intermediate_bitmap = paint_stroke(
                intermediate_bitmap, last_point, next, STROKE_WIDTH);
            last_point = next;
        }
    }
    int CHOSEN_KERNEL_SIZE = 7;
    final_bitmap = blur_bitmap(intermediate_bitmap, CHOSEN_KERNEL_SIZE);
    final_bitmap = image_util::resize_image(
        final_bitmap, 
        FINAL_BITMAP_WIDTH, 
        FINAL_BITMAP_HEIGHT, 
        1, true).get<flex_image>();
    return final_bitmap;
}

flex_image convert_stroke_based_drawing_to_bitmap(
    flex_list stroke_based_drawing, int row_number) {
    flex_list normalized_drawing = simplify_drawing(
        stroke_based_drawing, row_number);
    flex_image bitmap = rasterize(normalized_drawing);
    return bitmap;
}

static std::map<std::string,size_t> generate_column_index_map(
    const std::vector<std::string>& column_names) {
    std::map<std::string,size_t> index_map;
    for (size_t k=0; k < column_names.size(); ++k) {
        index_map[column_names[k]] = k;
    }
    return index_map;
}

gl_sframe _drawing_classifier_prepare_data(const gl_sframe &data,
                                           const std::string &feature) {
    DASSERT_TRUE(data.contains_column(feature));

    std::vector<flexible_type> bitmaps;
    auto column_index_map = generate_column_index_map(data.column_names());
    int row_number = 0;
    for (const auto& row: data.range_iterator()) {
        const flexible_type &strokes = row[column_index_map[feature]];
        flex_list current_stroke_based_drawing = strokes.to<flex_list>();
        bitmaps.push_back(convert_stroke_based_drawing_to_bitmap(
            current_stroke_based_drawing, row_number)
        );
        row_number++;
    }
    gl_sarray bitmaps_sarray(bitmaps);
    gl_sframe converted_sframe = gl_sframe(data);
    converted_sframe[feature] = bitmaps_sarray;
    converted_sframe.materialize();
    return converted_sframe;
}

} //drawing_classifier
} //turi
