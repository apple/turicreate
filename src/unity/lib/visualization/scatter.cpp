/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "scatter.hpp"

#include "process_wrapper.hpp"
#include "thread.hpp"
#include "vega_data.hpp"
#include "vega_spec.hpp"

#include <cmath>
#include <thread>

using namespace turi;
using namespace turi::visualization;

static std::string to_string(const flexible_type& ft) {
  switch (ft.get_type()) {
    case flex_type_enum::INTEGER:
      return std::to_string(ft.get<flex_int>());
    case flex_type_enum::FLOAT:
      return std::to_string(ft.get<flex_float>());
    default:
      throw std::runtime_error("Unexpected flexible_type type. Expected INTEGER or FLOAT.");
  }
}

scatter_result::scatter_result(gl_sframe sf) : m_sf(sf) {}

void scatter::init(gl_sframe sf){
  m_sf = sf;
}

std::shared_ptr<transformation_output> scatter::get(){
  return std::make_shared<scatter_result>(m_sf);
}

bool scatter::eof() const{
  return true;
}

flex_int scatter::get_rows_processed() const{
  return m_sf.size();
}

size_t scatter::get_batch_size() const{
  return m_sf.size();
}

std::string scatter_result::vega_column_data(bool sframe) const {
    std::stringstream ss;
    size_t x = 0;
    size_t size_list = m_sf.size();

    for (size_t i=0; i < size_list; i++) {

      if (m_sf["x"][i].get_type() == flex_type_enum::UNDEFINED ||
          m_sf["y"][i].get_type() == flex_type_enum::UNDEFINED) {
        continue;
      }

      if (m_sf["x"][i].get_type() == flex_type_enum::FLOAT &&
          !std::isfinite(m_sf["x"][i].get<flex_float>())) {
        continue;
      }
      if (m_sf["y"][i].get_type() == flex_type_enum::FLOAT &&
          !std::isfinite(m_sf["y"][i].get<flex_float>())) {
        continue;
      }

      if(x !=  0){
        ss << ",";
      }

      ss << "{\"x\": " + to_string(m_sf["x"][i]) + ", \"y\": " + to_string(m_sf["y"][i]) + "}";

      x++;
    }

    return ss.str();
}


std::shared_ptr<Plot> turi::visualization::plot_scatter(const std::string& path_to_client,
                                       const gl_sarray& x,
                                       const gl_sarray& y,
                                       const std::string& xlabel,
                                       const std::string& ylabel,
                                       const std::string& title) {


  DASSERT_EQ(x.size(), y.size());

  std::stringstream ss;
  ss << scatter_spec(xlabel, ylabel, title);
  std::string scatter_specification = ss.str();

  double size_array = static_cast<double>(x.size());

  scatter sct;

  gl_sframe temp_sf;

  temp_sf["x"] = x;
  temp_sf["y"] = y;

  sct.init(temp_sf);

  std::shared_ptr<transformation_base> shared_unity_transformer = std::make_shared<scatter>(sct);
  return std::make_shared<Plot>(path_to_client, scatter_specification, shared_unity_transformer, size_array);

}

