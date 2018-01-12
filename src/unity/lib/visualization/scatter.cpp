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

void turi::visualization::show_scatter(const std::string& path_to_client,
                                       const gl_sarray& x,
                                       const gl_sarray& y,
                                       const std::string& xlabel,
                                       const std::string& ylabel,
                                       const std::string& title) {

  ::turi::visualization::run_thread([path_to_client, x, y, xlabel, ylabel, title]() {

    DASSERT_EQ(x.size(), y.size());

    process_wrapper ew(path_to_client);
    ew << scatter_spec(xlabel, ylabel, title);

    vega_data vd;

    for (size_t i=0; i<x.size(); i++) {
      if (x[i].get_type() == flex_type_enum::UNDEFINED ||
          y[i].get_type() == flex_type_enum::UNDEFINED) {
        // TODO: show undefined values
        // for now, just skip them
        continue;
      }

      // for now, skip NaN/inf as well.
      if (x[i].get_type() == flex_type_enum::FLOAT &&
          !std::isfinite(x[i].get<flex_float>())) {
        continue;
      }
      if (y[i].get_type() == flex_type_enum::FLOAT &&
          !std::isfinite(y[i].get<flex_float>())) {
        continue;
      }
      vd << "{\"x\": " + to_string(x[i]) + ", \"y\": " + to_string(y[i]) + "}";
    }

    ew << vd.get_data_spec(1.0 /* progress */);

  });

}
