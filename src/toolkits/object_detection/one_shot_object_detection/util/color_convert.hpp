/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <boost/gil/gil_all.hpp>

namespace boost {
namespace gil {
// Define a color conversion rule NB in the boost::gil namespace
// RGB to RGBA
template <>
void color_convert<rgb8_pixel_t, rgba8_pixel_t>(const rgb8_pixel_t& src,
                                                rgba8_pixel_t& dst) {
  get_color(dst, red_t()) = get_color(src, red_t());
  get_color(dst, green_t()) = get_color(src, green_t());
  get_color(dst, blue_t()) = get_color(src, blue_t());

  using alpha_channel_t = color_element_type<rgba8_pixel_t, alpha_t>::type;
  get_color(dst, alpha_t()) = channel_traits<alpha_channel_t>::max_value();
}

// RGBA to RGB
template <>
void color_convert<rgba8_pixel_t, rgb8_pixel_t>(const rgba8_pixel_t& src,
                                                rgb8_pixel_t& dst) {
  get_color(dst, red_t()) = get_color(src, red_t());
  get_color(dst, green_t()) = get_color(src, green_t());
  get_color(dst, blue_t()) = get_color(src, blue_t());
  // ignore the alpha channel
}

}  // namespace gil
}  // namespace boost
