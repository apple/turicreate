/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "boxes_and_whiskers.hpp"

#include "process_wrapper.hpp"
#include "thread.hpp"
#include "vega_data.hpp"
#include "vega_spec.hpp"

#include <parallel/lambda_omp.hpp>

#include <cmath>
#include <thread>

using namespace turi::visualization;

static const std::string x_name = "x";
static const std::string y_name = "y";

std::string boxes_and_whiskers_result::vega_column_data(bool sframe) const {
  std::stringstream ss;

  std::unordered_map<flexible_type, flexible_type> grouped = get_grouped();

  size_t i = 0;
  size_t size_list = grouped.size();
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

    ss << "{\"" << x_name << "\": ";
    ss << escape_string(xValue);

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

    if (i != (size_list - 1)) {
      ss << ",";
    }
    i++;
  }

  return ss.str();
}

void ::turi::visualization::show_boxes_and_whiskers(const std::string& path_to_client,
                                                    const gl_sarray& x,
                                                    const gl_sarray& y,
                                                    const std::string& xlabel,
                                                    const std::string& ylabel,
                                                    const std::string& title) {

  ::turi::visualization::run_thread([path_to_client, x, y, xlabel, ylabel, title]() {

    DASSERT_EQ(x.size(), y.size());


    process_wrapper ew(path_to_client);
    ew << boxes_and_whiskers_spec(xlabel, ylabel, title);

    boxes_and_whiskers bw;

    gl_sframe temp_sf;
    temp_sf[x_name] = x;
    temp_sf[y_name] = y;
    bw.init(temp_sf);
    while (ew.good()) {
      vega_data vd;
      auto result = bw.get();
      vd << result->vega_column_data();

      double num_rows_processed =  static_cast<double>(bw.get_rows_processed());
      double size_array = static_cast<double>(x.size());
      double percent_complete = num_rows_processed/size_array;
      ew << vd.get_data_spec(percent_complete);

      if (bw.eof()) {
        break;
      }
    }

  });

}
