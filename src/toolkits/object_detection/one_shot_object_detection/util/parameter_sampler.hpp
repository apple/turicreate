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
   * dz: distance of the object from the camera.
   * focal: focal length of the camera used.
   * coordinates: coordinates in the annotation.
   * transform: The transformation matrix built from the above parameters.
   */
  double get_theta();
  double get_phi();
  double get_gamma();
  size_t get_dz();
  double get_focal();
  flex_dict get_coordinates();
  Eigen::Matrix<float, 3, 3> get_transform();

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
  size_t dx_ = 0;
  size_t dy_ = 0;
  size_t dz_;
  double focal_;
  float center_x_;
  float center_y_;
  float bounding_box_width_;
  float bounding_box_height_;
  Eigen::Matrix<float, 3, 3> transform_;
  std::vector<Eigen::Vector3f> warped_corners_;

  /* Getters for some private parameters:
   * warped_corners: The four corners of the object in the warped image.
   */
  std::vector<Eigen::Vector3f> get_warped_corners();
  
  /* Setter for warped_corners, built after applying the transformation
   * matrix on the corners of the starter image.
   * Order of warped_corners is top_left, top_right, bottom_left, bottom_right
   */
  void set_warped_corners(const std::vector<Eigen::Vector3f> &warped_corners);
  void set_annotation_without_planar_translation();
};

}  // namespace one_shot_object_detection
}  // namespace turi

#endif  // TURI_PARAMETER_SAMPLER_H_