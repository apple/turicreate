/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/one_shot_object_detection/one_shot_object_detector.hpp>

#include <unity/toolkits/object_detection/object_detector.hpp>

// TODO: Clean up imports.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <numeric>
#include <queue>
#include <utility>
#include <vector>
#include <random>

#include <logger/assertions.hpp>
#include <logger/logger.hpp>
#include <random/random.hpp>

#include <boost/gil/gil_all.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>
#include <boost/gil/utilities.hpp>

#include <image/numeric_extension/perspective_projection.hpp>

#include <Eigen/Core>
#include <Eigen/Dense>

namespace turi {
namespace one_shot_object_detection {

/* A ParameterSweep class to randomly generate different samples of parameters
 * that can later be used to compute the transformation matrix necessary to 
 * create image projections.
 */
class ParameterSweep {
public:
  ParameterSweep(int width, int height) {
    width_ = width;
    height_ = height;
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

  int get_dz() {
    return dz_;
  }

  double get_focal() {
    return focal_;
  }

  void sample(long seed) {
    /* Barebones */
    theta_ = 0.0;
    phi_ = 0.0;
    gamma_ = 0.0;
    std::normal_distribution<double> focal_distribution((double)width_, focal_stdev_);
    focal_generator_.seed(seed+6);
    focal_ = focal_distribution(focal_generator_);
    dz_ = focal_;
  }

private:
  int width_;
  int height_;
  double focal_stdev_ = 40.0;
  std::default_random_engine focal_generator_;
  double theta_;
  double phi_;
  double gamma_;
  int dz_;
  double focal_;

};

gl_sframe _augment_data(gl_sframe data, gl_sframe backgrounds, long seed) {
  // TODO: Get input image from the data sframe.
  // TODO: Use backgrounds from the background SFrame.
  // TODO: Generalize 1024 and 676 to be the width and height of the image 
  //       passed in.
  ParameterSweep parameter_sampler = ParameterSweep(2*1024, 2*676);
  // TODO: Take n as input.
  int n = 1;
  for (int i = 0; i < n; i++) {
    parameter_sampler.sample(seed+i);

    Eigen::Matrix3f mat = get_transformation_matrix(2*1024, 2*676,
      parameter_sampler.get_theta(),
      parameter_sampler.get_phi(),
      parameter_sampler.get_gamma(),
      1024/2,
      676/2,
      parameter_sampler.get_dz(),
      parameter_sampler.get_focal());

    // TODO: Construct annotations that conform to the OD format, from the 
    // four warped corner points.

    boost::gil::rgb8_image_t starter_image, background;
    // TODO: Don't hardcode this.
    boost::gil::read_image("in-affine.jpg", starter_image, boost::gil::jpeg_tag());
    // TODO: Don't hardcode this. Fetch this from the backgrounds SFrame in a 
    //       loop.
    boost::gil::read_image("background.jpg", background, boost::gil::jpeg_tag());

    matrix3x3<double> M(mat.inverse());
    // TODO: Use a background.
    // TODO: Use a mask during superposition on a random background.
    boost::gil::rgb8_image_t transformed(boost::gil::rgb8_image_t::point_t(view(starter_image).dimensions()*2));
    boost::gil::fill_pixels(view(transformed),boost::gil::rgb8_pixel_t(255, 255, 255));
    boost::gil::resample_pixels(const_view(starter_image), view(transformed), M, boost::gil::bilinear_sampler());

    // TODO: Write these images into an SFrame that this function can return.
    std::string output_filename = "out-perspective-" + std::to_string(i) + ".jpg";
    write_view(output_filename, view(transformed), boost::gil::jpeg_tag());
  }
  // TODO: Return the augmented data once the SFrame is written.
  return data;
}

one_shot_object_detector::one_shot_object_detector() {
  model_.reset(new turi::object_detection::object_detector());
}

gl_sframe one_shot_object_detector::augment(gl_sframe data,
                                            std::string target_column_name,
                                            gl_sframe backgrounds,
                                            long seed){
  gl_sframe augmented_data = _augment_data(data, backgrounds, seed);
  // TODO: Call object_detector::train from here once we incorporate mxnet into
  // the C++ Object Detector.
  return augmented_data;
}
/* TODO: We probably don't need `evaluate` and `export_to_coreml` on the C++ 
         side for now, but it may not hurt to leave it here.
 */
variant_map_type one_shot_object_detector::evaluate(gl_sframe data, 
  std::string metric, std::map<std::string, flexible_type> options) {
  return model_->evaluate(data, metric, options);
}

std::shared_ptr<coreml::MLModelWrapper> one_shot_object_detector::export_to_coreml(
  std::string filename, std::map<std::string, flexible_type> options) {
  return model_->export_to_coreml(filename, options);
}

} // one_shot_object_detection
} // turi