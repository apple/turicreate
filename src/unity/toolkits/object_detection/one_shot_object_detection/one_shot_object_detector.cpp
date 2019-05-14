/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <random>

#include <boost/gil/gil_all.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>

#include <Eigen/Core>
#include <Eigen/Dense>

#include <image/image_util_impl.hpp>

#include <unity/toolkits/object_detection/object_detector.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/one_shot_object_detector.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/util/color_convert.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/util/mapping_function.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/util/parameter_sampler.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/util/quadrilateral_geometry.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/util/superposition.hpp>

namespace turi {
namespace one_shot_object_detection {

namespace data_augmentation {

flex_dict build_annotation( ParameterSampler &parameter_sampler,
                            size_t object_width,
                            size_t object_height,
                            long seed) {

  parameter_sampler.sample(seed);

  size_t original_top_left_x = 0;
  size_t original_top_left_y = 0;
  size_t original_top_right_x = object_width;
  size_t original_top_right_y = 0;
  size_t original_bottom_left_x = 0;
  size_t original_bottom_left_y = object_height;
  size_t original_bottom_right_x = object_width;
  size_t original_bottom_right_y = object_height;

  Eigen::Vector3f top_left_corner(3)   , top_right_corner(3);
  Eigen::Vector3f bottom_left_corner(3), bottom_right_corner(3);
  top_left_corner     << original_top_left_x    , original_top_left_y    , 1;
  top_right_corner    << original_top_right_x   , original_top_right_y   , 1;
  bottom_left_corner  << original_bottom_left_x , original_bottom_left_y , 1;
  bottom_right_corner << original_bottom_right_x, original_bottom_right_y, 1;

  auto normalize = [](Eigen::Vector3f corner) {
    corner[0] /= corner[2];
    corner[1] /= corner[2];
    corner[2] = 1.0;
    return corner;
  };
  
  Eigen::Matrix<float, 3, 3> mat = parameter_sampler.get_transform();

  const std::vector<Eigen::Vector3f> warped_corners = {
                                          normalize(mat * top_left_corner)   ,
                                          normalize(mat * top_right_corner)  ,
                                          normalize(mat * bottom_left_corner),
                                          normalize(mat * bottom_right_corner)
                                         };
  parameter_sampler.set_warped_corners(warped_corners);

  float min_x = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::min();
  float min_y = std::numeric_limits<float>::max();
  float max_y = std::numeric_limits<float>::min();
  for (auto corner: warped_corners) {
    min_x = std::min(min_x, corner[0]);
    max_x = std::max(max_x, corner[0]);
    min_y = std::min(min_y, corner[1]);
    max_y = std::max(max_y, corner[1]);
  }
  float center_x = (min_x + max_x) / 2;
  float center_y = (min_y + max_y) / 2;
  float bounding_box_width  = max_x - min_x;
  float bounding_box_height = max_y - min_y;

  flex_dict coordinates = {std::make_pair("x", center_x),
                           std::make_pair("y", center_y),
                           std::make_pair("width", bounding_box_width),
                           std::make_pair("height", bounding_box_height)
                          };
  flex_dict annotation = {std::make_pair("coordinates", coordinates),
                          std::make_pair("label", "placeholder")
                         };
  return annotation;
}

static std::map<std::string,size_t> generate_column_index_map(
    const std::vector<std::string>& column_names) {
    std::map<std::string,size_t> index_map;
    for (size_t k=0; k < column_names.size(); ++k) {
        index_map[column_names[k]] = k;
    }
    return index_map;
}

gl_sframe augment_data(const gl_sframe &data,
                       const std::string& image_column_name,
                       const std::string& target_column_name,
                       const gl_sarray &backgrounds,
                       long long seed) {
  auto column_index_map = generate_column_index_map(data.column_names());
  std::vector<flexible_type> annotations;
  std::vector<flexible_type> images;
  for (const auto& row: data.range_iterator()) {
    flex_image object = row[column_index_map[image_column_name]].to<flex_image>();
    std::string label = row[column_index_map[target_column_name]].to<flex_string>();
    if (!(object.is_decoded())) {
      decode_image_inplace(object);
    }
    int row_number = -1;
    for (const auto& background_ft: backgrounds.range_iterator()) {
      row_number++;
      flex_image flex_background = background_ft.to<flex_image>();
      if (!(flex_background.is_decoded())) {
        decode_image_inplace(flex_background);
      }
      size_t background_width = flex_background.m_width;
      size_t background_height = flex_background.m_height;
      size_t background_channels = flex_background.m_channels;
      
      ParameterSampler parameter_sampler = ParameterSampler(
                                              background_width, 
                                              background_height,
                                              (background_width-object.m_width)/2,
                                              (background_height-object.m_height)/2);

      flex_dict annotation = build_annotation(parameter_sampler, 
                                              object.m_width, object.m_height, 
                                              seed+row_number);

      DASSERT_TRUE(flex_background.get_image_data() != nullptr);
      DASSERT_TRUE(object.is_decoded());
      DASSERT_TRUE(flex_background.is_decoded());

      boost::gil::rgb8_image_t::view_t background_view = interleaved_view(
        background_width,
        background_height,
        (boost::gil::rgb8_pixel_t*)(flex_background.get_image_data()),
        background_channels * background_width // row length in bytes
        );

      images.push_back(
        create_synthetic_image(background_view, parameter_sampler, object)
      );
      annotations.push_back(annotation);
    }
  }


  const std::map<std::string, std::vector<flexible_type> >& augmented_data = {
    {"annotation", annotations},
    {"image", images}
  };
  gl_sframe augmented_data_out = gl_sframe(augmented_data);
  return augmented_data_out;
}

} // data_augmentation

one_shot_object_detector::one_shot_object_detector() {
  model_.reset(new turi::object_detection::object_detector());
}

gl_sframe one_shot_object_detector::augment(const gl_sframe &data,
                                            const std::string& target_column_name,
                                            const gl_sarray &backgrounds,
                                            std::map<std::string, flexible_type> &options){
  
  // TODO: Automatically infer the image column name, or throw error if you can't
  // This should just happen on the Python side.
  std::string image_column_name = "image";
  gl_sframe augmented_data = data_augmentation::augment_data(data,
                                                             image_column_name,
                                                             target_column_name,
                                                             backgrounds,
                                                             options["seed"]);
  // TODO: Call object_detector::train from here once we incorporate mxnet into
  // the C++ Object Detector.
  return augmented_data;
}

} // one_shot_object_detection
} // turi