/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_ONE_SHOT_MAPPING_FUNCTION_H_
#define TURI_ONE_SHOT_MAPPING_FUNCTION_H_

#include <Eigen/Core>
#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include <boost/gil/gil_all.hpp>

namespace boost {
namespace gil {

/* Matrix - Vector multiplication */
template <typename T, typename F>
boost::gil::point2<F> operator*(const boost::gil::point2<T>& p,
                                const Eigen::Matrix<F, 3, 3>& m) {
  float denominator = m(2, 0) * p.x + m(2, 1) * p.y + m(2, 2);
  if (denominator == 0) {
    // TODO: Figure out the right failure behavior for when denominator is 0.
    return boost::gil::point2<F>(0, 0);
  }
  return boost::gil::point2<F>(
      (m(0, 0) * p.x + m(0, 1) * p.y + m(0, 2)) / denominator,
      (m(1, 0) * p.x + m(1, 1) * p.y + m(1, 2)) / denominator);
}

/* Conforming to the MapFn concept required by Boost GIL, for Eigen::Matrix3f
 */
template <typename T>
struct mapping_traits;

template <typename F, typename F2>
boost::gil::point2<F> transform(const Eigen::Matrix<F, 3, 3>& mat,
                                const boost::gil::point2<F2>& src) {
  return src * mat;
}

template <typename F>
struct mapping_traits<Eigen::Matrix<F, 3, 3> > {
  using result_type = boost::gil::point2<F>;
};

}  // namespace gil
}  // namespace boost

#endif  // TURI_ONE_SHOT_MAPPING_FUNCTION_H_