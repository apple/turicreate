/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "show.hpp"

#include "boxes_and_whiskers.hpp"
#include "categorical_heatmap.hpp"
#include "heatmap.hpp"
#include "scatter.hpp"

#include <logger/logger.hpp>
#include <unity/lib/toolkit_function_macros.hpp>

static bool isString(const turi::gl_sarray& sa) {
  return sa.dtype() == turi::flex_type_enum::STRING;
}

static bool isNumeric(const turi::gl_sarray& sa) {
  return sa.dtype() == turi::flex_type_enum::INTEGER ||
         sa.dtype() == turi::flex_type_enum::FLOAT;
}

namespace turi {
  namespace visualization {

    std::shared_ptr<Plot> plot(const std::string& path_to_client,
              gl_sarray& x,
              gl_sarray& y,
              const std::string& xlabel,
              const std::string& ylabel,
              const std::string& title) {

        logprogress_stream << "Materializing X axis SArray..." << std::endl;
        x.materialize();
        logprogress_stream << "Materializing Y axis SArray..." << std::endl;
        y.materialize();
        logprogress_stream << "Done." << std::endl;

        const size_t size = x.size();

        size_t x_size = x.size();
        size_t y_size = y.size();

        if(x_size == 0 && y_size == 0){
          throw std::runtime_error("Nothing to show; X axis, and Y axis SArrays are empty.");
        }else if(x_size == 0){
          throw std::runtime_error("Nothing to show; X axis SArray is empty.");
        }else if(y_size == 0){
          throw std::runtime_error("Nothing to show; Y axis SArray is empty.");
        }

        if (size != y.size()) {
          throw std::runtime_error("Expected x and y axis SArrays to be the same length.");
        }

        if (isNumeric(x) && isNumeric(y)) {
          if (size <= 5000) {
            return plot_scatter(path_to_client, x, y, xlabel, ylabel, title);
          } else {
            return plot_heatmap(path_to_client, x, y, xlabel, ylabel, title);
          }
        } else if (isNumeric(x) && isString(y)) {
          // TODO -- actually show this with the axes the user asked for
          // but for now, just flip them

          return plot_boxes_and_whiskers(path_to_client, y, x, xlabel, ylabel, title);
        } else if (isNumeric(y) && isString(x)) {

          return plot_boxes_and_whiskers(path_to_client, x, y, xlabel, ylabel, title);
        } else if (isString(x) &&
                   isString(y)) {
          return plot_categorical_heatmap(path_to_client, x, y, xlabel, ylabel, title);
        } else {
          throw std::runtime_error("Unsupported combination of SArray dtypes for x and y. Currently supported are: [int, float, str].");
        }
    }
  }
}
