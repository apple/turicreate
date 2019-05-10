#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <random>

#include <boost/gil/gil_all.hpp>

#include <Eigen/Core>
#include <Eigen/Dense>

namespace turi {
namespace one_shot_object_detection {

namespace data_augmentation {

namespace quadrilateral_geometry {

class Line {
public:
  /* Equation of Line from two points */
  Line(Eigen::Vector3f P1, Eigen::Vector3f P2);
  bool side_of_line(size_t x, size_t y);

private:
  float m_a, m_b, m_c; // ax + by + c = 0
};

/* Returns true if (x,y) is inside a quadrilateral 
 * defined by corners, where corners are in cyclic 
 * order from top right to bottom left.
 */
bool is_in_quadrilateral(size_t x, size_t y, 
  const std::vector<Eigen::Vector3f> &corners);

/* mask is an image with all pixels set to black and
 * mask_complement is an image with all pixels set to white.
 * This function colors the pixels inside the convex quadrilateral defined by
 * corners with white for the mask and black for the mask_complement.
 */
void color_quadrilateral(const boost::gil::rgb8_image_t::view_t &mask_view, 
                         const boost::gil::rgb8_image_t::view_t &mask_complement_view, 
                         const std::vector<Eigen::Vector3f> &corners);

} // quadrilateral_geometry
} // data_augmentation
} // one_shot_object_detection
} // turi
