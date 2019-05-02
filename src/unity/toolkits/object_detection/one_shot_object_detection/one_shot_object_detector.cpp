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
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>

#include <Eigen/Core>
#include <Eigen/Dense>

#include <image/numeric_extension/perspective_projection.hpp>

#include <unity/toolkits/object_detection/object_detector.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/one_shot_object_detector.hpp>

namespace turi {
namespace one_shot_object_detection {

/* A ParameterSampler class to randomly generate different samples of parameters
 * that can later be used to compute the transformation matrix necessary to 
 * create image projections.
 */
class ParameterSampler {
public:
  ParameterSampler(size_t width, size_t height, size_t dx, size_t dy) {
    width_ = width;
    height_ = height;
    dx_ = dx;
    dy_ = dy;
  }

  double deg_to_rad(double angle) {
    return angle * M_PI / 180.0;
  }

  double get_theta() {
    return deg_to_rad(theta_);
  }

  double get_phi() {
    return deg_to_rad(phi_);
  }

  double get_gamma() {
    return deg_to_rad(gamma_);
  }

  size_t get_dz() {
    return dz_;
  }

  double get_focal() {
    return focal_;
  }

  Eigen::Matrix<float, 3, 3> get_transform() {
    return transform_;
  }

  void set_warped_corners(const std::vector<Eigen::Vector3f> &warped_corners) {
    warped_corners_ = warped_corners;
  }

  std::vector<Eigen::Vector3f> get_warped_corners() {
    return warped_corners_;
  }

  void sample(long seed) {
    double theta_mean, phi_mean, gamma_mean;
    std::srand(seed);
    theta_mean = theta_means_[std::rand() % theta_means_.size()];
    std::srand(seed+1);
    phi_mean = phi_means_[std::rand() % phi_means_.size()];
    std::srand(seed+2);
    gamma_mean = gamma_means_[std::rand() % gamma_means_.size()];
    std::normal_distribution<double> theta_distribution(theta_mean, angle_stdev_);
    std::normal_distribution<double> phi_distribution(phi_mean, angle_stdev_);
    std::normal_distribution<double> gamma_distribution(gamma_mean, angle_stdev_);
    std::normal_distribution<double> focal_distribution((double)width_, focal_stdev_);
    theta_generator_.seed(seed+3);
    theta_ = theta_distribution(theta_generator_);
    phi_generator_.seed(seed+4);
    phi_ = phi_distribution(phi_generator_);
    gamma_generator_.seed(seed+5);
    gamma_ = gamma_distribution(gamma_generator_);
    focal_generator_.seed(seed+6);
    focal_ = focal_distribution(focal_generator_);
    std::uniform_int_distribution<int> dz_distribution(
      std::max(width_, height_), max_depth_);
    dz_generator_.seed(seed+7);
    dz_ = focal_ + dz_distribution(dz_generator_);
    transform_ = warp_perspective::get_transformation_matrix(
      width_, height_, theta_, phi_, gamma_, dx_, dy_, dz_, focal_);
    warped_corners_.reserve(4);
  }

private:
  size_t width_;
  size_t height_;
  size_t max_depth_ = 13000;
  double angle_stdev_ = 20.0;
  double focal_stdev_ = 40.0;
  std::vector<double> theta_means_ = {-180.0, 0.0, 180.0};
  std::vector<double> phi_means_   = {-180.0, 0.0, 180.0};
  std::vector<double> gamma_means_ = {-180.0, -90.0, 0.0, 90.0, 180.0};
  std::default_random_engine theta_generator_;
  std::default_random_engine phi_generator_;
  std::default_random_engine gamma_generator_;
  std::default_random_engine dz_generator_;
  std::default_random_engine focal_generator_;
  double theta_;
  double phi_;
  double gamma_;
  size_t dx_;
  size_t dy_;
  size_t dz_;
  double focal_;
  Eigen::Matrix<float, 3, 3> transform_;
  std::vector<Eigen::Vector3f> warped_corners_;
};

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

  std::vector<Eigen::Vector3f> warped_corners = {
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


gl_sframe augment_data(gl_sframe data,
                       const std::string& target_column_name,
                       gl_sarray backgrounds,
                       long seed) {
  // TODO: Get input image from the data sframe.
  // TODO: Use backgrounds from the background SArray.
  // TODO: Generalize 1024 and 676 to be the width and height of the image 
  //       passed in.
  ParameterSampler parameter_sampler = ParameterSampler(2*1024, 2*676, 1024/2, 676/2);
  // TODO: Take n as input.
  size_t n = 1;
  std::vector<flexible_type> annotations;
  for (size_t i = 0; i < n; i++) {
    parameter_sampler.sample(seed+i);

    flex_dict annotation = build_annotation(parameter_sampler, 
                                            1024, 676, 
                                            seed+i);

    std::vector<Eigen::Vector3f> corners = parameter_sampler.get_warped_corners();

    boost::gil::rgb8_image_t starter_image, background;
    // TODO: Don't hardcode this.
    boost::gil::read_image("in-affine.jpg", starter_image, boost::gil::jpeg_tag());
    // TODO: Don't hardcode this. Fetch this from the backgrounds SFrame in a 
    //       loop.
    boost::gil::read_image("background.jpg", background, boost::gil::jpeg_tag());

    Eigen::Matrix<float, 3, 3> M = parameter_sampler.get_transform().inverse();
    // TODO: Use a background.
    // TODO: Use a mask during superposition on a random background.
    boost::gil::rgb8_image_t transformed(boost::gil::rgb8_image_t::point_t(view(starter_image).dimensions()*2));
    boost::gil::fill_pixels(view(transformed),boost::gil::rgb8_pixel_t(255, 255, 255));
    boost::gil::resample_pixels(const_view(starter_image), view(transformed), M, boost::gil::bilinear_sampler());

    // TODO: Write these images into an SFrame that this function can return.
    std::string output_filename = "out-perspective-" + std::to_string(i) + ".jpg";
    write_view(output_filename, view(transformed), boost::gil::jpeg_tag());
    annotations.push_back(annotation);
  }
  const std::map<std::string, std::vector<flexible_type> >& augmented_data = {
    {"annotation", annotations}
  };
  // TODO: Add an image column here that also has the augmented image.
  gl_sframe augmented_data_out = gl_sframe(augmented_data);
  return augmented_data_out;
}

}

one_shot_object_detector::one_shot_object_detector() {
  model_.reset(new turi::object_detection::object_detector());
}

gl_sframe one_shot_object_detector::augment(gl_sframe data,
                                            const std::string& target_column_name,
                                            gl_sarray backgrounds,
                                            std::map<std::string, flexible_type> options){
  
  gl_sframe augmented_data = data_augmentation::augment_data(data,
                                                             target_column_name,
                                                             backgrounds,
                                                             options["seed"]);
  // TODO: Call object_detector::train from here once we incorporate mxnet into
  // the C++ Object Detector.
  return augmented_data;
}

} // one_shot_object_detection
} // turi