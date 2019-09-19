/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <boost/gil/extension/numeric/affine.hpp>

#include <core/data/image/numeric_extension/perspective_projection.hpp>
#include <core/logging/assertions.hpp>
#include <limits>

#include <toolkits/object_detection/one_shot_object_detection/util/parameter_sampler.hpp>

/* A ParameterSampler class to randomly generate different samples of parameters
 * that can later be used to compute the transformation matrix necessary to
 * create image projections.
 */
namespace turi {
namespace one_shot_object_detection {

ParameterSampler::ParameterSampler(size_t starter_width, size_t starter_height)
    : starter_width_(starter_width), starter_height_(starter_height) {
      warped_corners_.reserve(4);
    }

double deg_to_rad(double angle) { return angle * M_PI / 180.0; }

/* Getters for all the parameters:
 * theta: rotation around the x axis.
 * phi: rotation around the y axis.
 * gamma: rotation around the z axis.
 * dx: translation along x axis.
 * dy: translation along y axis.  
 * dz: distance of the object from the camera.
 * focal: focal length of the camera used.
 * coordinates: coordinates in the annotation.
 * perspective_transform: The perspective transformation matrix built from the
 *                        above parameters.
 * affine_transform: The affine transformation matrix built from the
 *                   dx and dy translations.
 * warped_corners: The four corners of the object in the warped image.
 */
double ParameterSampler::get_theta() { return theta_; }
double ParameterSampler::get_phi() { return phi_; }
double ParameterSampler::get_gamma() { return gamma_; }

int ParameterSampler::get_dx() { return dx_; }
int ParameterSampler::get_dy() { return dy_; }
size_t ParameterSampler::get_dz() { return dz_; }

double ParameterSampler::get_focal() { return focal_; }

Eigen::Matrix<float, 3, 3> ParameterSampler::get_perspective_transform() {
  return perspective_transform_;
}

Eigen::Matrix<float, 3, 3> ParameterSampler::get_affine_transform() {
  return affine_transform_;
}

std::vector<Eigen::Vector3f> ParameterSampler::get_warped_corners() {
  return warped_corners_;
}

void ParameterSampler::compute_coordinates_from_warped_corners() {
  float min_x = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::min();
  float min_y = std::numeric_limits<float>::max();
  float max_y = std::numeric_limits<float>::min();
  for (const auto &corner : warped_corners_) {
    min_x = std::min(min_x, corner[0]);
    max_x = std::max(max_x, corner[0]);
    min_y = std::min(min_y, corner[1]);
    max_y = std::max(max_y, corner[1]);
  }
  center_x_ = (min_x + max_x) / 2;
  center_y_ = (min_y + max_y) / 2;
  bounding_box_width_ = max_x - min_x;
  bounding_box_height_ = max_y - min_y;
}

void ParameterSampler::perform_x_y_translation(size_t background_width,
    size_t background_height, std::mt19937 *engine_pointer) {
  int x_margin = static_cast<int>(bounding_box_width_/2);
  int y_margin = static_cast<int>(bounding_box_height_/2);
  if (background_width < x_margin || background_height < y_margin) {
    dx_ = 0;
    dy_ = 0;
  } else {
    std::uniform_int_distribution<int> final_center_x_distribution(
      x_margin, background_width - x_margin);
    std::uniform_int_distribution<int> final_center_y_distribution(
      y_margin, background_height - y_margin);
    int new_center_x = final_center_x_distribution(*engine_pointer);
    int new_center_y = final_center_y_distribution(*engine_pointer);
    dx_ = new_center_x - static_cast<int>(center_x_);
    dy_ = new_center_y - static_cast<int>(center_y_);
  }
  
  boost::gil::matrix3x2<double> affine = boost::gil::matrix3x2<double>::get_translate(
    boost::gil::point2<double>(dx_, dy_));
  affine_transform_ << affine.a, affine.c, affine.e,
                       affine.b, affine.d, affine.f,
                              0,        0,        1;
  
  std::transform(warped_corners_.begin(), warped_corners_.end(),
    warped_corners_.begin(), 
    [&affine](Eigen::Vector3f corner) -> Eigen::Vector3f {
      boost::gil::point2<double> corner_2d(corner(0), corner(1));
      boost::gil::point2<double> translated_corner = corner_2d * affine;
      Eigen::Vector3f answer(translated_corner.x, translated_corner.y, 1.0);
      return answer;
  });

  compute_coordinates_from_warped_corners();
}

flex_dict ParameterSampler::get_coordinates() {
  
  flex_dict coordinates = {std::make_pair("x", center_x_),
                           std::make_pair("y", center_y_),
                           std::make_pair("width", bounding_box_width_),
                           std::make_pair("height", bounding_box_height_)};
  return coordinates;
  
}

int generate_random_index(std::mt19937 *engine_pointer, int range) {
  DASSERT_GT(range, 0);
  std::uniform_int_distribution<int> index_distribution(0, range-1);
  return index_distribution(*engine_pointer);
}

void ParameterSampler::perform_perspective_transform() {
  size_t original_top_left_x = 0;
  size_t original_top_left_y = 0;
  size_t original_top_right_x = starter_width_;
  size_t original_top_right_y = 0;
  size_t original_bottom_left_x = 0;
  size_t original_bottom_left_y = starter_height_;
  size_t original_bottom_right_x = starter_width_;
  size_t original_bottom_right_y = starter_height_;

  Eigen::Vector3f top_left_corner(3), top_right_corner(3);
  Eigen::Vector3f bottom_left_corner(3), bottom_right_corner(3);
  top_left_corner << original_top_left_x, original_top_left_y, 1;
  top_right_corner << original_top_right_x, original_top_right_y, 1;
  bottom_left_corner << original_bottom_left_x, original_bottom_left_y, 1;
  bottom_right_corner << original_bottom_right_x, original_bottom_right_y, 1;

  auto normalize = [](Eigen::Vector3f corner) {
    corner[0] /= corner[2];
    corner[1] /= corner[2];
    corner[2] = 1.0;
    return corner;
  };
  
  perspective_transform_ = warp_perspective::get_transformation_matrix(
      starter_width_, starter_height_, theta_, phi_, gamma_, 0, 0, dz_, focal_);
  
  warped_corners_ = {
      normalize(perspective_transform_ * top_left_corner),
      normalize(perspective_transform_ * top_right_corner),
      normalize(perspective_transform_ * bottom_left_corner),
      normalize(perspective_transform_ * bottom_right_corner)
  };

  compute_coordinates_from_warped_corners();
}

/* Function to sample all the parameters needed to build a transform, and
 * then also build the transform.
 */
void ParameterSampler::sample(size_t background_width, size_t background_height,
                              size_t seed, size_t row_number) {
  double theta_mean, phi_mean, gamma_mean;
  std::seed_seq seed_seq = {static_cast<int>(seed), static_cast<int>(row_number)};
  std::mt19937 engine(seed_seq);
  
  theta_mean = theta_means_[generate_random_index(&engine, theta_means_.size())];
  phi_mean = phi_means_[generate_random_index(&engine, phi_means_.size())];
  gamma_mean = gamma_means_[generate_random_index(&engine, gamma_means_.size())];
  
  std::normal_distribution<double> theta_distribution(theta_mean, angle_stdev_);
  std::normal_distribution<double> phi_distribution(phi_mean, angle_stdev_);
  std::normal_distribution<double> gamma_distribution(gamma_mean, angle_stdev_);
  std::normal_distribution<double> focal_distribution(static_cast<double>(background_width),
                                                      focal_stdev_);

  theta_ = deg_to_rad(theta_distribution(engine));
  phi_ = deg_to_rad(phi_distribution(engine));
  gamma_ = deg_to_rad(gamma_distribution(engine));
  focal_ = focal_distribution(engine);
  std::uniform_int_distribution<int> dz_distribution(std::max(background_width, background_height),
                                                     max_depth_);
  dz_ = focal_ + dz_distribution(engine);
  perform_perspective_transform();
  perform_x_y_translation(background_width, background_height, &engine);
}

}  // namespace one_shot_object_detection
}  // namespace turi
