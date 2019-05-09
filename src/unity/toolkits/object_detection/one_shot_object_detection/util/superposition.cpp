/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>

#include <unity/toolkits/object_detection/one_shot_object_detection/util/mapping_function.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/util/superposition.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/util/quadrilateral_geometry.hpp>

#define BLACK boost::gil::rgb8_pixel_t(0,0,0)
#define WHITE boost::gil::rgb8_pixel_t(255,255,255)
#define RGBA_WHITE boost::gil::rgba8_pixel_t(255,255,255,0)

namespace turi {
namespace one_shot_object_detection {

namespace data_augmentation {

namespace superposition {

void superimpose_image_rgb(const boost::gil::rgb8_image_t::view_t &masked,
                           const boost::gil::rgb8_image_t::view_t &mask,
                           const boost::gil::rgb8_image_t::view_t &transformed,
                           const boost::gil::rgb8_image_t::view_t &mask_complement,
                           const boost::gil::rgb8_image_t::view_t &background) {
  for (int y = 0; y < masked.height(); ++y) {
    auto masked_row_it = masked.row_begin(y);
    auto mask_row_it = mask.row_begin(y);
    auto transformed_row_it = transformed.row_begin(y);
    auto mask_complement_row_it = mask_complement.row_begin(y);
    auto background_row_it = background.row_begin(y);
    for (int x = 0; x < masked.width(); ++x) {
      masked_row_it[x][0] = (mask_row_it[x][0]/255 * transformed_row_it[x][0] + 
        mask_complement_row_it[x][0]/255 * background_row_it[x][0]);
      masked_row_it[x][1] = (mask_row_it[x][1]/255 * transformed_row_it[x][1] + 
        mask_complement_row_it[x][1]/255 * background_row_it[x][1]);
      masked_row_it[x][2] = (mask_row_it[x][2]/255 * transformed_row_it[x][2] + 
        mask_complement_row_it[x][2]/255 * background_row_it[x][2]);
    }
  }
}


flex_image create_synthetic_rgb_image(const boost::gil::rgb8_image_t::view_t &background_view,
                                      ParameterSampler &parameter_sampler,
                                      const flex_image &object) {
  Eigen::Matrix<float, 3, 3> M = parameter_sampler.get_transform().inverse();
  boost::gil::rgb8_image_t::view_t starter_image_view = interleaved_view(
    object.m_width,
    object.m_height,
    (boost::gil::rgb8_pixel_t*) (object.get_image_data()),
    object.m_channels * object.m_width // row length in bytes
  );
  boost::gil::rgb8_image_t mask(boost::gil::rgb8_image_t::point_t(background_view.dimensions()));
  boost::gil::rgb8_image_t mask_complement(boost::gil::rgb8_image_t::point_t(background_view.dimensions()));
  // mask_complement = 1 - mask
  fill_pixels(view(mask), BLACK);
  fill_pixels(view(mask_complement), WHITE);
  quadrilateral_geometry::color_quadrilateral(view(mask), view(mask_complement), 
    parameter_sampler.get_warped_corners());
  boost::gil::rgb8_image_t transformed(boost::gil::rgb8_image_t::point_t(background_view.dimensions()));
  fill_pixels(view(transformed), WHITE);
  resample_pixels(starter_image_view, view(transformed), M, boost::gil::bilinear_sampler());
  boost::gil::rgb8_image_t masked(boost::gil::rgba8_image_t::point_t(background_view.dimensions()));
  fill_pixels(view(masked), WHITE);
  // Superposition:
  // mask * warped + (1-mask) * background
  superimpose_image_rgb(view(masked), view(mask), view(transformed), 
                        view(mask_complement), background_view);
  return flex_image(masked);
}


void superimpose_image_rgba(const boost::gil::rgba8_image_t::view_t &masked,
                            const boost::gil::rgba8_image_t::view_t &transformed,
                            const boost::gil::rgba8_image_t::view_t &background) {
  for (int y = 0; y < masked.height(); ++y) {
    auto masked_row_it = masked.row_begin(y);
    auto transformed_row_it = transformed.row_begin(y);
    auto background_row_it = background.row_begin(y);
    auto AoverB = [](float C_a, float C_b, float alpha_a, float alpha_b){
      return ((C_a * alpha_a + C_b * alpha_b * (1 - alpha_a))/(
                alpha_a + alpha_b * (1 - alpha_a)));
    };
    for (int x = 0; x < masked.width(); ++x) {
      float alpha_a = transformed_row_it[x][3]/255;
      float alpha_b = background_row_it[x][3]/255;
      masked_row_it[x][0] = AoverB(transformed_row_it[x][0], background_row_it[x][0], alpha_a, alpha_b);
      masked_row_it[x][1] = AoverB(transformed_row_it[x][1], background_row_it[x][1], alpha_a, alpha_b);
      masked_row_it[x][2] = AoverB(transformed_row_it[x][2], background_row_it[x][2], alpha_a, alpha_b);
      masked_row_it[x][3] = AoverB(transformed_row_it[x][3], background_row_it[x][3], alpha_a, alpha_b);;
    }
  }
}

flex_image create_synthetic_rgba_image(const boost::gil::rgb8_image_t::view_t &background_view, 
                                       ParameterSampler &parameter_sampler,
                                       const flex_image &object) {
  Eigen::Matrix<float, 3, 3> M = parameter_sampler.get_transform().inverse();
  boost::gil::rgba8_image_t::view_t starter_image_view = interleaved_view(
    object.m_width,
    object.m_height,
    (boost::gil::rgba8_pixel_t*) (object.get_image_data()),
    object.m_channels * object.m_width // row length in bytes
  );
  boost::gil::rgba8_image_t background_rgba(boost::gil::rgba8_image_t::point_t(background_view.dimensions()));
  // The following line uses the color_convert conversion to convert the 
  // background from RGB to RGBA
  boost::gil::copy_and_convert_pixels(
    background_view,
    boost::gil::view(background_rgba)
  );
  boost::gil::rgba8_image_t transformed(boost::gil::rgba8_image_t::point_t(background_view.dimensions()));
  fill_pixels(view(transformed), RGBA_WHITE);
  resample_pixels(starter_image_view, view(transformed), M, boost::gil::bilinear_sampler());
  boost::gil::rgba8_image_t masked(boost::gil::rgba8_image_t::point_t(background_view.dimensions()));
  fill_pixels(view(masked), RGBA_WHITE);
  superimpose_image_rgba(view(masked), view(transformed), 
                         view(background_rgba));
  return flex_image(masked);
}
} // superposition
} // data_augmentation
} // one_shot_object_detection
} // turi