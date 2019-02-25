/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <time.h>
#include <limits.h>
#include <math.h>
#include <iomanip>
#include <logger/assertions.hpp>
#include <image/image_type.hpp>
#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#endif // __APPLE__
#include "data_preparation.hpp"
#include "random/random.hpp"
#include "util/sys_util.hpp"

namespace turi {
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
            new_stroke.push_back(new_point);
        }
        // Apply RDP Line Algorithm
        flex_list compressed_stroke = ramer_douglas_peucker(new_stroke, 2.0);
        simplified_drawing.push_back(compressed_stroke);
    }
    return simplified_drawing;
}

flex_list rasterize(flex_list simplified_drawing) {
#ifdef __APPLE__
    int MAC_OS_STRIDE = 64;
    int INTERMEDIATE_BITMAP_WIDTH = 256;
    int INTERMEDIATE_BITMAP_HEIGHT = 256;
    int FINAL_BITMAP_WIDTH = 28;
    int FINAL_BITMAP_HEIGHT = 28;
    float STROKE_WIDTH = 20.0;
    size_t num_strokes = simplified_drawing.size();
    flex_list final_bitmap; // 1 x 28 x 28
    CGColorSpaceRef grayscale = CGColorSpaceCreateDeviceGray();
    CGContextRef bitmap_context = CGBitmapContextCreate(
        NULL, INTERMEDIATE_BITMAP_WIDTH, INTERMEDIATE_BITMAP_HEIGHT, 
        8, 0, grayscale, kCGImageAlphaNone);
    CGContextSetRGBStrokeColor(bitmap_context, 1.0, 1.0, 1.0, 1.0);
    CGAffineTransform transform = CGAffineTransformIdentity;
    CGMutablePathRef path = CGPathCreateMutable();
    for (size_t i = 0; i < num_strokes; i++) {
        const flex_list &stroke = simplified_drawing[i].get<flex_list>();
        const flex_dict &start_point_dict = stroke[0].get<flex_dict>();
        size_t num_points_in_stroke = stroke.size();
        Point P; make_point(&P, start_point_dict);
        // @TODO: When we make a dylib, we'll need some sophisticated 
        // iOS/macOS checking here to figure out whether we need to 
        // subtract 256 or not........
        CGPathMoveToPoint(path, &transform, 
            P.x, ((float)(INTERMEDIATE_BITMAP_WIDTH))-P.y);
        for (size_t j = 1; j < num_points_in_stroke; j++) {
            const flex_dict &point_dict = stroke[j].get<flex_dict>();
            make_point(&P, point_dict);
            CGPathAddLineToPoint(path, &transform, 
                P.x, ((float)(INTERMEDIATE_BITMAP_WIDTH))-P.y);
        }
    }
    CGContextSetLineWidth(bitmap_context, STROKE_WIDTH);
    CGContextBeginPath(bitmap_context);
    CGContextAddPath(bitmap_context, path);
    CGContextStrokePath(bitmap_context);
    CGPathRelease(path);
    CGImageRef image = CGBitmapContextCreateImage(bitmap_context);
    CGRect rectangle = CGRectMake(0.0, 0.0, 
        ((float)(INTERMEDIATE_BITMAP_WIDTH)), 
        ((float)(INTERMEDIATE_BITMAP_HEIGHT)));
    CGImageRef cropped_image = CGImageCreateWithImageInRect(image, rectangle);
    CGContextRef scale_28_context = CGBitmapContextCreate(NULL, 
        FINAL_BITMAP_WIDTH, FINAL_BITMAP_HEIGHT, 8, 0, grayscale, 
        kCGImageAlphaNone);
    CGRect new_rect = CGRectMake(0.0, 0.0, 
        ((float)(FINAL_BITMAP_WIDTH)), ((float)(FINAL_BITMAP_HEIGHT)));
    CGContextDrawImage(scale_28_context, new_rect, cropped_image);
    CGImageRef cropped_image_gray = CGBitmapContextCreateImage(scale_28_context);
    CFDataRef pixel_data = CGDataProviderCopyData(
        CGImageGetDataProvider(cropped_image_gray));
    const uint8_t* data_ptr = CFDataGetBytePtr(pixel_data);
    flex_list outer_layer;
    for (size_t row = 0; row < FINAL_BITMAP_WIDTH; row++) {
        flex_list current_row;
        for (size_t col = 0; col < FINAL_BITMAP_HEIGHT; col++) {
            // check platform here okay
            uint32_t val = (uint32_t)(data_ptr[row * MAC_OS_STRIDE + col]);
            current_row.push_back(val/255.0);
        }
        outer_layer.push_back(current_row);
    }
    final_bitmap.push_back(outer_layer);
    return final_bitmap;
#endif // __APPLE__
    return simplified_drawing;
}

flex_list convert_stroke_based_drawing_to_bitmap(
    flex_list stroke_based_drawing) {
    flex_list normalized_drawing = simplify_stroke(stroke_based_drawing);
    flex_list bitmap = rasterize(normalized_drawing);
    return bitmap;
}

gl_sframe _drawing_recognition_prepare_data(const gl_sframe &data,
                                            const std::string &feature,
                                            const std::string &target) {
    DASSERT_TRUE(data.contains_column(feature));
    DASSERT_TRUE(data.contains_column(target));

    gl_sarray bitmaps = data[feature].apply([](const flexible_type &strokes) {
        flex_list current_stroke_based_drawing = strokes.to<flex_list>();
        flex_list current_bitmap = convert_stroke_based_drawing_to_bitmap(
            current_stroke_based_drawing);
        return current_bitmap;
    }, flex_type_enum::LIST);

    gl_sframe converted_sframe = gl_sframe(data);
    converted_sframe.replace_add_column(bitmaps, "bitmap");
    
    converted_sframe.materialize();
    return converted_sframe;
}    

} //drawing_recognition
} //turi
