/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <Eigen/Core>

namespace warp_perspective {

Eigen::MatrixXf get_2D_to_3D(int width, int height) {
  Eigen::MatrixXf A1(4,3);
  A1 << 1, 0,  -1*(float)width/2,
        0, 1,  -1*(float)height/2,
        0, 0,         1,
        0, 0,         1;
  return A1;
}

Eigen::MatrixXf get_rotation(float theta, float phi, float gamma) {
  Eigen::Matrix4f Rx, Ry, Rz, R;
  Rx << 1,               0,                0, 0,
        0, std::cos(theta), -1*std::sin(theta), 0,
        0, std::sin(theta),  std::cos(theta), 0,
        0,               0,                0, 1;
  Ry << std::cos(phi), 0, -1*std::sin(phi), 0,
                    0, 1,              0, 0,
        std::sin(phi), 0,  std::cos(phi), 0,
                    0, 0,              0, 1;
  Rz << std::cos(gamma), -1*std::sin(gamma), 0, 0,
        std::sin(gamma),  std::cos(gamma), 0, 0,
                      0,                0, 1, 0,
                      0,                0, 0, 1;
  R = (Rx * Ry) * Rz;
  return R;
}

Eigen::MatrixXf get_translation(int dx, int dy, int dz) {
  Eigen::Matrix4f T;
  T << 1, 0, 0, dx,
       0, 1, 0, dy,
       0, 0, 1, dz,
       0, 0, 0, 1;
  return T;
}

Eigen::MatrixXf get_3D_to_2D(float focal, int width, int height) {
  Eigen::MatrixXf A2(3,4);
  A2 << focal,     0,  (float)width/2, 0,
            0, focal, (float)height/2, 0,
            0,     0,               1, 0;
  return A2;
}

Eigen::Matrix<float, 3, 3> get_transformation_matrix(
  int width, int height,
  float theta, float phi, float gamma,
  int dx, int dy, int dz,
  float focal) {
  Eigen::MatrixXf A1  = get_2D_to_3D(width, height);
  Eigen::Matrix4f R   = get_rotation(theta, phi, gamma);
  Eigen::Matrix4f T   = get_translation(dx, dy, dz);
  Eigen::MatrixXf A2  = get_3D_to_2D(focal, width, height);
  return (A2 * (T * (R * A1)));
}

}
