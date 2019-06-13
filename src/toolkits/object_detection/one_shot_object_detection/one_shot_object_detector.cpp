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

#include <Eigen/Core>
#include <Eigen/Dense>

#include <core/data/image/image_util_impl.hpp>

#include <model_server/lib/image_util.hpp>
#include <toolkits/object_detection/object_detector.hpp>
#include <toolkits/object_detection/one_shot_object_detection/one_shot_object_detector.hpp>
#include <toolkits/object_detection/one_shot_object_detection/util/color_convert.hpp>
#include <toolkits/object_detection/one_shot_object_detection/util/mapping_function.hpp>
#include <toolkits/object_detection/one_shot_object_detection/util/parameter_sampler.hpp>
#include <toolkits/object_detection/one_shot_object_detection/util/superposition.hpp>

namespace turi {
namespace one_shot_object_detection {
namespace data_augmentation {

flex_dict build_annotation( ParameterSampler &parameter_sampler,
                            std::string label,
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
                          std::make_pair("label", label)
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

boost::gil::rgba8_image_t::view_t create_starter_image_view(flex_image &object_input) {
  DASSERT_TRUE(object_input.is_decoded());
  flex_image object = image_util::resize_image(object_input,
                                               object_input.m_width,
                                               object_input.m_height,
                                               4,
                                               true).to<flex_image>();
  DASSERT_TRUE(object.is_decoded());
  DASSERT_EQ(object.m_channels, 4);
  boost::gil::rgba8_image_t::view_t starter_image_view = interleaved_view(
    object.m_width,
    object.m_height,
    (boost::gil::rgba8_pixel_t*) (object.get_image_data()),
    object.m_channels * object.m_width // row length in bytes
  );
  return starter_image_view;
}

gl_sframe augment_data(const gl_sframe &data,
                       const std::string& image_column_name,
                       const std::string& target_column_name,
                       const gl_sarray &backgrounds,
                       long long seed,
                       bool verbose) {
  size_t backgrounds_size = backgrounds.size();
  size_t total_augmented_rows = data.size() * backgrounds_size;
  table_printer table( { {"Images Augmented", 0}, {"Elapsed Time", 0}, {"Percent Complete", 0} } );
  if (verbose) {
    logprogress_stream << "Augmenting input images using " << backgrounds.size() << " background images." << std::endl;
    table.print_header();
  }

  auto column_index_map = generate_column_index_map(data.column_names());
  std::vector<flexible_type> annotations;
  std::vector<flexible_type> images;
  int input_row_index = -1;
  for (const auto& row: data.range_iterator()) {
    input_row_index++;
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

      flex_dict annotation = build_annotation(parameter_sampler, label,
                                              object.m_width, object.m_height,
                                              seed+row_number);

      DASSERT_TRUE(flex_background.get_image_data() != nullptr);
      DASSERT_TRUE(object.is_decoded());
      DASSERT_TRUE(flex_background.is_decoded());

      boost::gil::rgba8_image_t::view_t starter_image_view = create_starter_image_view(object);

      boost::gil::rgb8_image_t::view_t background_view = interleaved_view(
        background_width,
        background_height,
        (boost::gil::rgb8_pixel_t*)(flex_background.get_image_data()),
        background_channels * background_width // row length in bytes
        );

      images.push_back(
        create_synthetic_image(starter_image_view, background_view, parameter_sampler, object)
      );
      annotations.push_back(annotation);

      if (verbose) {
        std::ostringstream d;
        // For pretty printing, floor percent done
        // resolution to the nearest .25% interval.  Do this by multiplying by
        // 400, then do integer division by the total size, then float divide
        // by 4.0
        int augmented_rows_completed = (input_row_index * backgrounds_size) + row_number;
        d << augmented_rows_completed * 400 / total_augmented_rows / 4.0 << '%';
        table.print_progress_row(augmented_rows_completed,
                                  augmented_rows_completed,
                                  progress_time(), d.str());
      }
    }
  }
  if (verbose) {
    table.print_footer();
  }


  const std::map<std::string, std::vector<flexible_type> >& augmented_data = {
    {target_column_name, annotations},
    {image_column_name, images}
  };
  gl_sframe augmented_data_out = gl_sframe(augmented_data);
  return augmented_data_out;
}

} // data_augmentation

one_shot_object_detector::one_shot_object_detector() {
  model_.reset(new turi::object_detection::object_detector());
}

gl_sframe one_shot_object_detector::augment(const gl_sframe &data,
                                            const std::string& image_column_name,
                                            const std::string& target_column_name,
                                            const gl_sarray &backgrounds,
                                            std::map<std::string, flexible_type> &options){

  // TODO: Automatically infer the image column name, or throw error if you can't
  // This should just happen on the Python side.
  gl_sframe augmented_data = data_augmentation::augment_data(data,
                                                             image_column_name,
                                                             target_column_name,
                                                             backgrounds,
                                                             options["seed"],
                                                             options["verbose"]);
  // TODO: Call object_detector::train from here once we incorporate mxnet into
  // the C++ Object Detector.
  return augmented_data;
}

} // one_shot_object_detection
} // turi
