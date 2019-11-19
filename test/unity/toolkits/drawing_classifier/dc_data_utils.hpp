/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DRAWING_CLASSIFIER_TESTING_DATA_UTILS_H_
#define TURI_DRAWING_CLASSIFIER_TESTING_DATA_UTILS_H_

#include <algorithm>

#include <model_server/lib/image_util.hpp>
#include <toolkits/drawing_classifier/dc_data_iterator.hpp>

namespace turi {
namespace drawing_classifier {

constexpr size_t IMAGE_WIDTH = 28;
constexpr size_t IMAGE_HEIGHT = 28;

class drawing_data_generator {

 public:
  /** Builds an SFrame with columns "test_image" and "test_targets".
   *  Creates an SFrame with num_rows number of rows where each row has
   *  a drawing (a grayscale 28x28 image)
   *  and a corresponding target, which is the row index modulo
   *  unique_labels.size().
   */
  drawing_data_generator(bool is_bitmap_based, size_t num_rows,
      const flex_list &unique_labels,
      const std::string& target_name = "test_target",
      const std::string& feature_name = "test_feature")
      : is_bitmap_based_(is_bitmap_based),
        num_rows_(num_rows),
        unique_labels_(unique_labels),
        target_column_name_(target_name),
        feature_column_name_(feature_name) {

    if (is_bitmap_based_) {
      generate_bitmap_based_data();
    } else {
      generate_stroke_based_data();
    }
  }


  /* Get the unique labels based on the generated data */
  flex_list get_unique_labels() const {
    flex_list expected_class_labels;
    if (num_rows_ < unique_labels_.size()) {
      expected_class_labels.insert(
        expected_class_labels.end(),
        unique_labels_.begin(),
        unique_labels_.begin() + num_rows_);
    } else {
      expected_class_labels = unique_labels_;
    }
    return expected_class_labels;
  }

  void set_class_labels(flex_list class_labels) {
    params_.class_labels = std::move(class_labels);
  }

  data_iterator::parameters get_iterator_params() const {
    return params_;
  }

  gl_sframe get_data() const {
    return params_.data;
  }

  std::string get_feature_column_name() const { return feature_column_name_; };

  std::string get_target_column_name() const { return target_column_name_; };

 private:

  /** Builds an SFrame with feature and target columns.
   *  Creates an SFrame with num_rows number of rows where each row has 
   *  a drawing (a grayscale 28x28 image)
   *  and a corresponding target, which is the row index modulo 
   *  unique_labels.size().
   */
  void generate_bitmap_based_data() {
    flex_list images(num_rows_);
    flex_list labels(num_rows_);

    std::vector<unsigned char> buffer(IMAGE_WIDTH * IMAGE_HEIGHT * 1);
    for (size_t ii = 0; ii < num_rows_; ++ii) {

      // Each pixel has pixel value equal to the row index (modulo 256).
      std::fill(buffer.begin(), buffer.end(),
                static_cast<unsigned char>(ii % 256));
      images[ii] = flex_image(reinterpret_cast<char*>(buffer.data()),
                             IMAGE_HEIGHT, IMAGE_WIDTH, 1, buffer.size(),
                             IMAGE_TYPE_CURRENT_VERSION,
                             static_cast<int>(Format::RAW_ARRAY));

      // Each image has a label, which is the row index mod unique_labels.size().
      labels[ii] = unique_labels_[ii % unique_labels_.size()];
    }

    params_.target_column_name = target_column_name_;
    params_.feature_column_name = feature_column_name_;
    params_.data =  gl_sframe({
        {feature_column_name_, gl_sarray(images)},
        {target_column_name_, gl_sarray(labels)},
    });
    params_.shuffle = false;
    params_.class_labels = get_unique_labels();
  }

  flex_list create_stroke_based_drawing(size_t num_strokes_in_drawing) {
    
    flex_list drawing;
    size_t delta_x = 4;
    size_t delta_y = 8;
    size_t starting_point_scale = 10 * num_strokes_in_drawing;

    for (size_t stroke_id = 0; stroke_id < num_strokes_in_drawing; stroke_id++) {

      flex_list stroke;
      size_t x_value = num_strokes_in_drawing + stroke_id * starting_point_scale;
      size_t y_value = num_strokes_in_drawing + stroke_id * starting_point_scale;
      size_t num_points_in_stroke = stroke_id;

      for (size_t point_id = 0; point_id < num_points_in_stroke; point_id++) {

        flex_dict point;
        point.push_back(std::make_pair("x", x_value));
        point.push_back(std::make_pair("y", y_value));

        x_value += delta_x;
        y_value += delta_y;

        stroke.push_back(point);
      }
      drawing.push_back(stroke);
    }
    return drawing; 
  }


  /** Builds an SFrame with feature and target columns.
   *  Creates an SFrame with num_rows number of rows where each row has a
   *  stroke-based drawing.
   *  Every drawing is a sequence of strokes, 
   *  Every stroke is a collection of (x,y) points represented as a flex_dict. 
   */
  void generate_stroke_based_data() {
    flex_list drawings(num_rows_);
    flex_list labels(num_rows_);
    
    for (size_t ii = 0; ii < num_rows_; ++ii) {

      /** Easy optimization here in case we want to reduce test time:
       *  Add (ii % 100) or something */
      drawings[ii] = create_stroke_based_drawing(ii);

      // Each image has a label, which is the row index mod unique_labels.size().
      labels[ii] = unique_labels_[ii % unique_labels_.size()];
    }

    params_.target_column_name = target_column_name_;
    params_.feature_column_name = feature_column_name_;
    params_.data =  gl_sframe({
        {feature_column_name_, gl_sarray(drawings)},
        {target_column_name_, gl_sarray(labels)},
    });
    params_.shuffle = false;
    params_.class_labels = get_unique_labels();
  }

 private:
  bool is_bitmap_based_;
  data_iterator::parameters params_;
  size_t num_rows_;
  flex_list unique_labels_;
  std::string target_column_name_ = "test_target";
  std::string feature_column_name_ = "test_feature";
};

}  // namespace drawing_classifier
}  // namespace turi

#endif //TURI_DRAWING_CLASSIFIER_TESTING_DATA_UTILS_H_