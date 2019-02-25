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

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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

flex_list simplify_drawing(flex_list raw_drawing) {
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

void swap_points(Point *P1, Point *P2) {
    float temp_x, temp_y;
    if (P1->x > P2->x) {
        temp_x = P1->x;
        temp_y = P1->y;
        P1->x = P2->x;
        P1->y = P2->y;
        P2->x = temp_x;
        P2->y = temp_y;
    }
}

bool in_bounds(int x, int y, int dim) {
    return (x >= 0 && x < dim && y >= 0 && y < dim);
}

flex_nd_vec paint_stroke(
    flex_nd_vec bitmap, Point start, Point end, float stroke_width) {
    swap_points(&start, &end);
    float slope = (end.y - start.y)/(end.x - start.x);
    int x1 = (int)(start.x);
    int y1 = (int)(start.y);
    int x2 = (int)(end.x);
    int y2 = (int)(end.y);
    int pad = (int)(stroke_width/2);
    size_t dimension = bitmap.shape()[1];
    if (x1 == x2) {
        // just paint the y axis
        int min_y = MIN(y1, y2);
        int max_y = MAX(y1, y2);
        for (int y = min_y; y < max_y; y++) {
            for (int dx = -pad; dx < pad; dx++) {
                for (int dy = -pad; dy < pad; dy++) {
                    // figure out how to do this with the flex_list
                    if (in_bounds(x1+dx, y+dy, dimension)) {
                        bitmap[(y+dy)*dimension + (x1+dx)] = 1.0;
                    }
                }
            }
        }
    } else {
        for (int x = x1; x < x2; x++) {
            int y = (int)(slope * (x - x1) + y1);
            for (int dx = -pad; dx < pad; dx++) {
                for (int dy = -pad; dy < pad; dy++) {
                    if (in_bounds(x+dx, y+dy, dimension)) {
                        bitmap[(y+dy) * dimension + (x+dx)] = 1.0;
                    }
                }
            }
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
                blurred_bitmap[index] = (int)(255*bitmap[index]);
                continue;
            }
            double sum_for_blur = 0.0;
            for (int dr = -pad; dr <= pad; dr++) {
                for (int dc = -pad; dc <= pad; dc++) {
                    sum_for_blur += bitmap[(row+dr) * dimension + (col+dc)];
                }
            }
            blurred_bitmap[index] = (int)(255*sum_for_blur/(ksize*ksize));
        }
    }
    uint8_t image_data[dimension * dimension];
    for (int idx=0; idx < dimension * dimension; idx++) {
        image_data[idx] = ((uint8_t)(blurred_bitmap[idx]));
    }
    return flex_image((const char *)image_data,
        dimension, dimension, 1, dimension*dimension, 1, 2);
}

flex_image rasterize(flex_list simplified_drawing) {
    flex_image final_bitmap; // 1 x 28 x 28
    size_t num_strokes = simplified_drawing.size();
    int INTERMEDIATE_BITMAP_WIDTH = 256;
    int INTERMEDIATE_BITMAP_HEIGHT = 256;
    int FINAL_BITMAP_WIDTH = 28;
    int FINAL_BITMAP_HEIGHT = 28;
    float STROKE_WIDTH = 20.0;
    int MAC_OS_STRIDE = 64;
#ifdef __APPLE__
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
    CGImageRef cropped_image_gray = CGBitmapContextCreateImage(
        scale_28_context);
    CFDataRef pixel_data = CGDataProviderCopyData(
        CGImageGetDataProvider(cropped_image_gray));
    const uint8_t* data_ptr = CFDataGetBytePtr(pixel_data);
    uint8_t real_data[FINAL_BITMAP_WIDTH * FINAL_BITMAP_HEIGHT];
    for (size_t row = 0; row < FINAL_BITMAP_WIDTH; row++) {
        for (size_t col = 0; col < FINAL_BITMAP_HEIGHT; col++) {
            // check platform here
            real_data[row*FINAL_BITMAP_WIDTH+col] = (uint8_t)(
                data_ptr[row * MAC_OS_STRIDE + col]);
        }
    }
    return flex_image((const char *)real_data, 
        FINAL_BITMAP_WIDTH, FINAL_BITMAP_HEIGHT, 1, 
        FINAL_BITMAP_WIDTH * FINAL_BITMAP_HEIGHT, 1, 2);
#endif // __APPLE__
    Point last_point;
    std::vector<size_t> intermediate_bitmap_shape {1, (size_t)INTERMEDIATE_BITMAP_WIDTH, (size_t)INTERMEDIATE_BITMAP_HEIGHT};
    flex_nd_vec intermediate_bitmap(intermediate_bitmap_shape, 0.0);
    for (size_t i = 0; i < num_strokes; i++) {
        const flex_list &stroke = simplified_drawing[i].get<flex_list>();
        const flex_dict &start_point_dict = stroke[0].get<flex_dict>();
        size_t num_points_in_stroke = stroke.size();
        Point start; make_point(&start, start_point_dict);
        last_point = start;
        for (size_t j = 1; j < num_points_in_stroke; j++) {
            const flex_dict &next_point_dict = stroke[j].get<flex_dict>();
            Point next; make_point(&next, next_point_dict);
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
    flex_list stroke_based_drawing) {
    flex_list normalized_drawing = simplify_drawing(stroke_based_drawing);
    flex_image bitmap = rasterize(normalized_drawing);
    return bitmap;
}

gl_sframe _drawing_recognition_prepare_data(const gl_sframe &data,
                                            const std::string &feature,
                                            const std::string &target) {
    DASSERT_TRUE(data.contains_column(feature));
    DASSERT_TRUE(data.contains_column(target));
    
    gl_sarray bitmaps = data[feature].apply([](const flexible_type &strokes) {
        flex_list current_stroke_based_drawing = strokes.to<flex_list>();
        flex_image current_bitmap = convert_stroke_based_drawing_to_bitmap(
            current_stroke_based_drawing);
        return current_bitmap;
    }, flex_type_enum::IMAGE);

    gl_sframe converted_sframe = gl_sframe(data);
    converted_sframe.replace_add_column(bitmaps, feature);
    
    converted_sframe.materialize();
    return converted_sframe;
}

} //drawing_recognition
} //turi
