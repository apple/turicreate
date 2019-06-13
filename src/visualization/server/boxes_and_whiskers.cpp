/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "boxes_and_whiskers.hpp"

#include "batch_size.hpp"
#include "process_wrapper.hpp"
#include "thread.hpp"
#include "vega_data.hpp"
#include "vega_spec.hpp"

#include <parallel/lambda_omp.hpp>
#include <unity/lib/visualization/escape.hpp>
#include <unity/lib/visualization/plot.hpp>
#include <unity/lib/visualization/transformation.hpp>

#include <cmath>
#include <thread>

using namespace turi::visualization;

static const std::string x_name = "x";
static const std::string y_name = "y";

std::string boxes_and_whiskers_result::vega_column_data(bool sframe) const {
  std::stringstream ss;

  std::unordered_map<flexible_type, flexible_type> grouped = get_grouped();

  size_t i = 0;

  for (const auto& pair : grouped) {
    // if x is missing, nothing to plot -- skip for now
    // TODO: eventually we should probably have an "undefined" bin
    if (pair.first.get_type() == flex_type_enum::UNDEFINED) {
      continue;
    }

    const auto& xValue = pair.first.get<flex_string>();
    const auto& yValues = pair.second.get<flex_vec>();

    // TODO - for now, skip values with nan/inf
    bool isFinite = true;
    for (const auto& yValue : yValues) {
      if (!std::isfinite(yValue)) {
        isFinite = false;
      }
    }
    if (!isFinite) {
      continue;
    }

    if (i != 0) {
      ss << ",";
    }

    ss << "{\"" << x_name << "\": ";
    ss << extra_label_escape(xValue);

    ss << ",\"min\": ";
    ss << yValues[0];
    ss << ",\"lower quartile\": ";
    ss << yValues[1];
    ss << ",\"median\": ";
    ss << yValues[2];
    ss << ",\"upper quartile\": ";
    ss << yValues[3];
    ss << ",\"max\": ";
    ss << yValues[4];

    ss << "}";

    i++;
  }

  return ss.str();
}

std::shared_ptr<Plot> turi::visualization::plot_boxes_and_whiskers(
                                                    const gl_sarray& x,
                                                    const gl_sarray& y,
                                                    const flexible_type& xlabel,
                                                    const flexible_type& ylabel,
                                                    const flexible_type& title) {


  std::string boxes_and_whiskers_specification = boxes_and_whiskers_spec(xlabel, ylabel, title);

  double size_array = static_cast<double>(x.size());

  boxes_and_whiskers bw;

  gl_sframe temp_sf;
  temp_sf[x_name] = x;
  temp_sf[y_name] = y;

  bw.init(temp_sf, batch_size(x, y));

  std::shared_ptr<transformation_base> shared_unity_transformer = std::make_shared<boxes_and_whiskers>(bw);
  return std::make_shared<Plot>(boxes_and_whiskers_specification, shared_unity_transformer, size_array);
}
