/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <core/data/image/numeric_extension/perspective_projection.hpp>

#include <toolkits/object_detection/one_shot_object_detection/util/parameter_sampler.hpp>

/* A ParameterSampler class to randomly generate different samples of parameters
 * that can later be used to compute the transformation matrix necessary to
 * create image projections.
 */
namespace turi {
namespace one_shot_object_detection {

ParameterSampler::ParameterSampler(size_t width, size_t height, size_t dx,
                                   size_t dy)
    : width_(width), height_(height), dx_(dx), dy_(dy) {}

double deg_to_rad(double angle) { return angle * M_PI / 180.0; }

/* Getters for all the parameters:
 * theta: rotation around the x axis.
 * phi: rotation around the y axis.
 * gamma: rotation around the z axis.
 * dz: distance of the object from the camera.
 * focal: focal length of the camera used.
 * transform: The transformation matrix built from the above parameters
 * warped_corners: The four corners of the object in the warped image
 */
double ParameterSampler::get_theta() { return deg_to_rad(theta_); }

double ParameterSampler::get_phi() { return deg_to_rad(phi_); }

double ParameterSampler::get_gamma() { return deg_to_rad(gamma_); }

size_t ParameterSampler::get_dz() { return dz_; }

double ParameterSampler::get_focal() { return focal_; }

Eigen::Matrix<float, 3, 3> ParameterSampler::get_transform() {
  return transform_;
}

std::vector<Eigen::Vector3f> ParameterSampler::get_warped_corners() {
  return warped_corners_;
}

/* Setter for warped_corners, built after applying the transformation
 * matrix on the corners of the starter image.
 * Order of warped_corners is top_left, top_right, bottom_left, bottom_right
 */
void ParameterSampler::set_warped_corners(
    const std::vector<Eigen::Vector3f> &warped_corners) {
  warped_corners_ = warped_corners;
  // swap last two entries to make the corners cyclic.
  warped_corners_[2] = warped_corners[3];
  warped_corners_[3] = warped_corners[2];
}

/* Function to sample all the parameters needed to build a transform, and
 * then also build the transform.
 */
void ParameterSampler::sample(long seed) {
  double theta_mean, phi_mean, gamma_mean;
  std::srand(seed);
  theta_mean = theta_means_[std::rand() % theta_means_.size()];
  std::srand(seed + 1);
  phi_mean = phi_means_[std::rand() % phi_means_.size()];
  std::srand(seed + 2);
  gamma_mean = gamma_means_[std::rand() % gamma_means_.size()];
  std::normal_distribution<double> theta_distribution(theta_mean, angle_stdev_);
  std::normal_distribution<double> phi_distribution(phi_mean, angle_stdev_);
  std::normal_distribution<double> gamma_distribution(gamma_mean, angle_stdev_);
  std::normal_distribution<double> focal_distribution((double)width_,
                                                      focal_stdev_);
  theta_generator_.seed(seed + 3);
  theta_ = theta_distribution(theta_generator_);
  phi_generator_.seed(seed + 4);
  phi_ = phi_distribution(phi_generator_);
  gamma_generator_.seed(seed + 5);
  gamma_ = gamma_distribution(gamma_generator_);
  focal_generator_.seed(seed + 6);
  focal_ = focal_distribution(focal_generator_);
  std::uniform_int_distribution<int> dz_distribution(std::max(width_, height_),
                                                     max_depth_);
  dz_generator_.seed(seed + 7);
  dz_ = focal_ + dz_distribution(dz_generator_);
  transform_ = warp_perspective::get_transformation_matrix(
      width_, height_, theta_, phi_, gamma_, dx_, dy_, dz_, focal_);
  warped_corners_.reserve(4);
}

}  // namespace one_shot_object_detection
}  // namespace turi
