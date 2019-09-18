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

#include <core/data/flexible_type/flexible_type.hpp>

namespace turi {
namespace one_shot_object_detection {

/* A ParameterSampler class to randomly generate different samples of parameters
 * that can later be used to compute the transformation matrix necessary to
 * create image projections.
 */
class ParameterSampler {
 public:
  ParameterSampler(size_t starter_width, size_t starter_height);

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
  double get_theta();
  double get_phi();
  double get_gamma();
  int get_dx();
  int get_dy();
  size_t get_dz();
  double get_focal();
  flex_dict get_coordinates();
  Eigen::Matrix<float, 3, 3> get_perspective_transform();
  Eigen::Matrix<float, 3, 3> get_affine_transform();

  /* Function to sample all the parameters needed to build a transform, and
   * then also build the transform.
   */
  void sample(size_t background_width, size_t background_height,
              size_t seed, size_t row_number);

 private:
  size_t starter_width_;
  size_t starter_height_;
  size_t max_depth_ = 13000;
  double angle_stdev_ = 20.0;
  double focal_stdev_ = 40.0;
  std::vector<double> theta_means_ = {-180.0, 0.0, 180.0};
  std::vector<double> phi_means_ = {-180.0, 0.0, 180.0};
  std::vector<double> gamma_means_ = {-180.0, -90.0, 0.0, 90.0, 180.0};
  double theta_;
  double phi_;
  double gamma_;
  int dx_ = 0;
  int dy_ = 0;
  size_t dz_;
  double focal_;
  float center_x_;
  float center_y_;
  float bounding_box_width_;
  float bounding_box_height_;

  Eigen::Matrix<float, 3, 3> affine_transform_;
  Eigen::Matrix<float, 3, 3> perspective_transform_;
  std::vector<Eigen::Vector3f> warped_corners_;

  /* Getters for some private parameters:
   * warped_corners: The four corners of the object in the warped image.
   */
  std::vector<Eigen::Vector3f> get_warped_corners();
  
  void compute_coordinates_from_warped_corners();
  void perform_x_y_translation(size_t background_width, size_t background_height, std::mt19937 *engine_pointer);
  void perform_perspective_transform();
};

}  // namespace one_shot_object_detection
}  // namespace turi

#endif  // TURI_PARAMETER_SAMPLER_H_