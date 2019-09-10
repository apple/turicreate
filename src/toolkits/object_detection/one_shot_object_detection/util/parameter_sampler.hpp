/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_PARAMETER_SAMPLER_H_
#define TURI_PARAMETER_SAMPLER_H_

#include <Eigen/Core>
#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

namespace turi {
namespace one_shot_object_detection {

/* A ParameterSampler class to randomly generate different samples of parameters
 * that can later be used to compute the transformation matrix necessary to
 * create image projections.
 */
class ParameterSampler {
 public:
  ParameterSampler(size_t width, size_t height, size_t dx, size_t dy);

  /* Getters for all the parameters:
   * theta: rotation around the x axis.
   * phi: rotation around the y axis.
   * gamma: rotation around the z axis.
   * dz: distance of the object from the camera.
   * focal: focal length of the camera used.
   * transform: The transformation matrix built from the above parameters.
   * warped_corners: The four corners of the object in the warped image.
   */
  double get_theta();
  double get_phi();
  double get_gamma();
  size_t get_dz();
  double get_focal();
  Eigen::Matrix<float, 3, 3> get_transform();
  std::vector<Eigen::Vector3f> get_warped_corners();

  /* Setter for warped_corners, built after applying the transformation
   * matrix on the corners of the starter image.
   * Order of warped_corners is top_left, top_right, bottom_left, bottom_right
   */
  void set_warped_corners(const std::vector<Eigen::Vector3f> &warped_corners);

  /* Function to sample all the parameters needed to build a transform, and
   * then also build the transform.
   */
  void sample(long seed);

 private:
  size_t width_;
  size_t height_;
  size_t max_depth_ = 13000;
  double angle_stdev_ = 20.0;
  double focal_stdev_ = 40.0;
  std::vector<double> theta_means_ = {-180.0, 0.0, 180.0};
  std::vector<double> phi_means_ = {-180.0, 0.0, 180.0};
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

}  // namespace one_shot_object_detection
}  // namespace turi

#endif  // TURI_PARAMETER_SAMPLER_H_