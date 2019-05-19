/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/object_detection/one_shot_object_detection/util/quadrilateral_geometry.hpp>

static constexpr size_t TRANSPARENT = 0;
static constexpr size_t OPAQUE = 255;

namespace turi {
namespace one_shot_object_detection {
namespace data_augmentation {

Line::Line(Eigen::Vector3f P1, Eigen::Vector3f P2) {
  float x1 = P1[0], y1 = P1[1];
  float x2 = P2[0], y2 = P2[1];
  m_a = (y2 - y1) / (x2 - x1);
  m_b = -1;
  m_c = y1 - (x1 * (y2 - y1) / (x2 - x1));
}

bool Line::side_of_line(size_t x, size_t y) {
  return (m_a*x + m_b*y + m_c > 0);
}

bool is_in_quadrilateral(size_t x, size_t y, 
  const std::vector<Eigen::Vector3f> &warped_corners) {
  size_t num_true = 0;
  for (size_t index = 0; index < warped_corners.size(); index++) {
    auto left_corner = warped_corners[index % warped_corners.size()];
    auto right_corner = warped_corners[(index+1) % warped_corners.size()];
    Line L = Line(left_corner, right_corner);
    num_true += (L.side_of_line(x, y));
  }
  return (num_true == 2);
}

void color_quadrilateral(const boost::gil::rgba8_image_t::view_t &transformed_view, 
                         const std::vector<Eigen::Vector3f> &warped_corners) {
  size_t min_x = std::numeric_limits<size_t>::max();
  size_t max_x = std::numeric_limits<size_t>::min();
  size_t min_y = std::numeric_limits<size_t>::max();
  size_t max_y = std::numeric_limits<size_t>::min();
  for (auto corner: warped_corners) {
    min_x = std::min(min_x, (size_t)(corner[0]));
    max_x = std::max(max_x, (size_t)(corner[0]));
    min_y = std::min(min_y, (size_t)(corner[1]));
    max_y = std::max(max_y, (size_t)(corner[1]));
  }
  for (size_t y = min_y; y < max_y; ++y) {
    auto transformed_row_iterator = transformed_view.row_begin(y);
    for (size_t x = min_x; x < max_x; ++x) {
      if (is_in_quadrilateral(x, y, warped_corners)) {
        get_color(transformed_row_iterator[x], boost::gil::alpha_t()) = std::min((size_t)get_color(transformed_row_iterator[x], boost::gil::alpha_t()), OPAQUE);
      } else {
        get_color(transformed_row_iterator[x], boost::gil::alpha_t()) = TRANSPARENT;
      }
    }
  }
}

} // data_augmentation
} // one_shot_object_detection
} // turi
