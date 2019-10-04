/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

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

ParameterSampler::ParameterSampler(size_t starter_width, size_t starter_height,
                                   size_t dx, size_t dy)
    : starter_width_(starter_width),
      starter_height_(starter_height),
      dx_(dx),
      dy_(dy) {}

double deg_to_rad(double angle) { return angle * M_PI / 180.0; }

std::vector<double> vector_deg_to_rad(
    const std::vector<double> &means_in_degrees) {
  std::vector<double> means_in_radians;
  means_in_radians.reserve(means_in_degrees.size());
  for (double mean : means_in_degrees) {
    means_in_radians.push_back(deg_to_rad(mean));
  }
  return means_in_radians;
}

/* Getters for all the parameters:
 * theta: rotation around the x axis.
 * phi: rotation around the y axis.
 * gamma: rotation around the z axis.
 * dz: distance of the object from the camera.
 * focal: focal length of the camera used.
 * transform: The transformation matrix built from the above parameters
 * warped_corners: The four corners of the object in the warped image
 */
double ParameterSampler::get_theta() { return theta_; }

double ParameterSampler::get_phi() { return phi_; }

double ParameterSampler::get_gamma() { return gamma_; }

std::vector<double> ParameterSampler::get_theta_means() {
  return vector_deg_to_rad(theta_means_);
}

std::vector<double> ParameterSampler::get_phi_means() {
  return vector_deg_to_rad(phi_means_);
}

std::vector<double> ParameterSampler::get_gamma_means() {
  return vector_deg_to_rad(gamma_means_);
}

double ParameterSampler::get_theta_stdev() { return deg_to_rad(angle_stdev_); }

double ParameterSampler::get_phi_stdev() { return deg_to_rad(angle_stdev_); }

double ParameterSampler::get_gamma_stdev() { return deg_to_rad(angle_stdev_); }

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

int generate_random_index(std::mt19937 *engine_pointer, int range) {
  DASSERT_GT(range, 0);
  std::uniform_int_distribution<int> index_distribution(0, range - 1);
  return index_distribution(*engine_pointer);
}

/* Function to sample all the parameters needed to build a transform, and
 * then also build the transform.
 */
void ParameterSampler::sample(size_t background_width, size_t background_height,
                              size_t seed, size_t row_number) {
  double theta_mean, phi_mean, gamma_mean;
  std::seed_seq seed_seq = {static_cast<int>(seed),
                            static_cast<int>(row_number)};
  std::mt19937 engine(seed_seq);

  theta_mean =
      theta_means_[generate_random_index(&engine, theta_means_.size())];
  phi_mean = phi_means_[generate_random_index(&engine, phi_means_.size())];
  gamma_mean =
      gamma_means_[generate_random_index(&engine, gamma_means_.size())];

  std::normal_distribution<double> theta_distribution(theta_mean, angle_stdev_);
  std::normal_distribution<double> phi_distribution(phi_mean, angle_stdev_);
  std::normal_distribution<double> gamma_distribution(gamma_mean, angle_stdev_);
  std::normal_distribution<double> focal_distribution(
      static_cast<double>(background_width), focal_stdev_);

  theta_ = deg_to_rad(theta_distribution(engine));
  phi_ = deg_to_rad(phi_distribution(engine));
  gamma_ = deg_to_rad(gamma_distribution(engine));
  focal_ = focal_distribution(engine);
  std::uniform_int_distribution<int> dz_distribution(
      std::max(background_width, background_height), max_depth_);
  dz_ = focal_ + dz_distribution(engine);
  transform_ = warp_perspective::get_transformation_matrix(
      starter_width_, starter_height_, theta_, phi_, gamma_, dx_, dy_, dz_,
      focal_);
  warped_corners_.reserve(4);
}

}  // namespace one_shot_object_detection
}  // namespace turi
