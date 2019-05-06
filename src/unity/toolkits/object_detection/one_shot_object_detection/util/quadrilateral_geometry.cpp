#include <unity/toolkits/object_detection/one_shot_object_detection/util/quadrilateral_geometry.hpp>

#define BLACK boost::gil::rgb8_pixel_t(0,0,0)
#define WHITE boost::gil::rgb8_pixel_t(255,255,255)

namespace turi {
namespace one_shot_object_detection {

namespace data_augmentation {

namespace quadrilateral_geometry {

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
  float min_x = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::min();
  float min_y = std::numeric_limits<float>::max();
  float max_y = std::numeric_limits<float>::min();
  for (auto corner: warped_corners) {
    min_x = std::min(min_x, corner[0]);
    max_x = std::max(max_x, corner[0]);
    min_y = std::min(min_y, corner[1]);
    max_y = std::max(max_y, corner[1]);
  }
  if (x < min_x || x > max_x || y < min_y || y > max_y) {
    return false;
  }
  size_t num_true = 0;
  for (size_t index = 0; index < warped_corners.size(); index++) {
    auto left_corner = warped_corners[index % warped_corners.size()];
    auto right_corner = warped_corners[(index+1) % warped_corners.size()];
    Line L = Line(left_corner, right_corner);
    num_true += (L.side_of_line(x, y));
  }
  return (num_true == 2);
}

void color_quadrilateral(const boost::gil::rgb8_image_t::view_t &mask_view, 
                         const boost::gil::rgb8_image_t::view_t &mask_complement_view, 
                         const std::vector<Eigen::Vector3f> &warped_corners) {
  for (int y = 0; y < mask_view.height(); ++y) {
    auto mask_row_iterator = mask_view.row_begin(y);
    auto mask_complement_row_iterator = mask_complement_view.row_begin(y);
    for (int x = 0; x < mask_view.width(); ++x) {
      if (is_in_quadrilateral(x, y, warped_corners)) {
        mask_row_iterator[x] = WHITE;
        mask_complement_row_iterator[x] = BLACK;
      }
    }
  }
}

} // quadrilateral_geometry
} // data_augmentation
} // one_shot_object_detection
} // turi
