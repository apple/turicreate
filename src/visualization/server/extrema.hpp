/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef __TC_VIS_EXTREMA
#define __TC_VIS_EXTREMA

#include <core/data/sframe/gl_sframe.hpp>

#include <ostream>

namespace turi {
namespace visualization {

template<typename T>
class extrema {
  private:
    T m_max = std::numeric_limits<T>::min();
    T m_min = std::numeric_limits<T>::max();

  public:
    void update(const extrema<T>& value) {
      this->update(value.get_min());
      this->update(value.get_max());
    }

    void update(const T& value) {
      if (value < m_min) {
        m_min = value;
      }
      if (value > m_max) {
        m_max = value;
      }
    }

    T get_max() const {
      // if you are getting values, you should've put at least one value in first.
      // hitting this assertion means the extrema is probably initialized to [FLOAT_MAX, FLOAT_MIN].
      DASSERT_GE(m_max, m_min);
      return m_max;
    }

    T get_min() const {
      // if you are getting values, you should've put at least one value in first.
      // hitting this assertion means the extrema is probably initialized to [FLOAT_MAX, FLOAT_MIN].
      DASSERT_GE(m_max, m_min);
      return m_min;
    }

    bool operator==(const extrema<T>& other) const {
      return m_max == other.m_max && m_min == other.m_min;
    }

    friend std::ostream& operator<<(std::ostream& stream, const extrema<T>& ex) {
      stream << "[" << ex.m_min << ", " << ex.m_max << "]";
      return stream;
    }
};

template<typename T>
struct bounding_box {
  extrema<T> x;
  extrema<T> y;
  void update(const bounding_box<T>& value) {
    this->x.update(value.x);
    this->y.update(value.y);
  }

  bool operator==(const bounding_box<T>& other) const {
    return x == other.x && y == other.y;
  }

  friend std::ostream& operator<<(std::ostream& stream, const bounding_box<T>& bb) {
    stream << "[" << bb.x << ", " << bb.y << "]";
    return stream;
  }
};

}}

#endif
