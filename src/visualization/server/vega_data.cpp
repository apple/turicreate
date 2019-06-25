/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "vega_data.hpp"
#include "vega_spec.hpp"
#include <core/data/flexible_type/flexible_type_spirit_parser.hpp>
#include <algorithm>

namespace turi {
namespace visualization {

vega_data::vega_data() {
  m_spec << "{\"name\": \"source_2\", \"values\": [";
}

std::string vega_data::get_data_spec(double progress) {
  m_spec << "], \"progress\": "+std::to_string(progress)+" }";
  return m_spec.str();
}

vega_data& vega_data::operator<<(const std::string& vega_string) {
  if (!m_has_spec) {
    m_spec << vega_string;
    m_has_spec = true;
  } else {
    m_spec << "," << vega_string;
  }
  return *this;
}

} // visualization
} // turi
