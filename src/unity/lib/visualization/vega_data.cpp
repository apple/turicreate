/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "vega_data.hpp"
#include "vega_spec.hpp"
#include <flexible_type/flexible_type_spirit_parser.hpp>
#include <algorithm>

namespace turi {
namespace visualization {

vega_data::vega_data() {
  m_spec << "{\"data_spec\": {\"name\": \"source_2\", \"values\": [";
}

std::string vega_data::get_data_spec(double progress) {
  m_spec << "], \"progress\": "+std::to_string(progress)+" }}\n";
  return m_spec.str();
}

std::string vega_data::create_sframe_spec(
    size_t i,
    size_t num_rows,
    flex_type_enum type,
    std::string element_title,
    const std::shared_ptr<transformation_output>& result
) {
  std::stringstream ss;
  ss << "{\"a\": " << std::to_string(i) << ",";
  std::string title = extra_label_escape(element_title);
  ss << "\"title\": " << title << ",";
  ss << "\"num_row\": " << num_rows << ",";

  switch (type) {
    case flex_type_enum::INTEGER:
    case flex_type_enum::FLOAT:
    case flex_type_enum::STRING:
    {
      auto h_result = std::dynamic_pointer_cast<sframe_transformation_output>(result);
      ss << h_result->vega_summary_data();
      ss << "}";
      break;
    }
    default:
      throw std::runtime_error("Unexpected dtype. SFrame plot expects int, float or str dtypes.");
  }
  return ss.str();
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
