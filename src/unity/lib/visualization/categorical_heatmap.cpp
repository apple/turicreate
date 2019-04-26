/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "categorical_heatmap.hpp"

#include "batch_size.hpp"
#include "process_wrapper.hpp"
#include "thread.hpp"
#include "vega_data.hpp"
#include "vega_spec.hpp"

#include <parallel/lambda_omp.hpp>
#include <unity/lib/visualization/escape.hpp>
#include <unity/lib/visualization/plot.hpp>
#include <unity/lib/visualization/transformation.hpp>

#include <thread>

using namespace turi::visualization;

std::vector<categorical_heatmap_result> categorical_heatmap::split_input(size_t num_threads) {
  // TODO - do we need to do anything here? perhaps not.
  return std::vector<categorical_heatmap_result>(num_threads);
}

void categorical_heatmap::merge_results(std::vector<categorical_heatmap_result>& transformers) {
  for (const auto& other : transformers) {
    this->m_transformer->combine(other);
  }
}

std::string categorical_heatmap_result::vega_column_data(bool sframe) const {
  auto items_list = emit().get<flex_dict>();
  size_t size_list = items_list.size();

  // First, build up a map of xLabel -> (yLabel -> count)
  std::vector<std::string> xLabelsOrder, yLabelsOrder;
  std::set<std::string> xLabels, yLabels;
  std::unordered_map<std::string, std::unordered_map<std::string, flex_int>> counts;
  for (size_t i=0; i<size_list; i++) {
    const std::pair<flexible_type, flexible_type>& pair = items_list[i];
    const flex_list& values = pair.first.get<flex_list>();
    flex_int count = pair.second.get<flex_int>();

    flex_string xLabel, yLabel;
    if (values[0].get_type() == flex_type_enum::UNDEFINED) {
      xLabel = "(null)";
    } else {
      xLabel = values[0].get<flex_string>();
    }
    if (values[1].get_type() == flex_type_enum::UNDEFINED) {
      yLabel = "(null)";
    } else {
      yLabel = values[1].get<flex_string>();
    }

    // insert x & y labels into set & ordering if needed
    if (xLabels.find(xLabel) == xLabels.end()) {
      xLabels.insert(xLabel);
      xLabelsOrder.push_back(xLabel);
    }
    if (yLabels.find(yLabel) == yLabels.end()) {
      yLabels.insert(yLabel);
      yLabelsOrder.push_back(yLabel);
    }

    // insert count into the dictionary
    counts[xLabel][yLabel] = count;
  }

  // Finally, output JSON for each pair
  std::stringstream ss;
  size_t x = 0;
  for (const std::string& xLabel : xLabelsOrder) {
    for (const std::string& yLabel : yLabelsOrder) {
      // C++ unordered map returns default if key is missing,
      // and 0 is what we want in that case, so we're good.
      flex_int count = counts[xLabel][yLabel]; 
      if(x !=  0){
        ss << ",";
      }

      ss << "{\"x\": ";
      ss << extra_label_escape(xLabel);
      ss << ", \"y\": ";
      ss << extra_label_escape(yLabel);
      ss << ", \"count\": ";
      ss << count;
      ss << "}";

      x++;
    }
  }  

  return ss.str();
}


std::shared_ptr<Plot> turi::visualization::plot_categorical_heatmap(
                                                      const gl_sarray& x,
                                                      const gl_sarray& y,
                                                      const flexible_type& xlabel,
                                                      const flexible_type& ylabel,
                                                      const flexible_type& title) {

    std::stringstream ss;
    ss << categorical_heatmap_spec(xlabel, ylabel, title);
    std::string categorical_heatmap_specification = ss.str();

    double size_array = static_cast<double>(x.size());

    categorical_heatmap hm;

    gl_sframe temp_sf;

    temp_sf["x"] = x;
    temp_sf["y"] = y;

    hm.init(temp_sf, batch_size(x, y));

    std::shared_ptr<transformation_base> shared_unity_transformer = std::make_shared<categorical_heatmap>(hm);
    return std::make_shared<Plot>(categorical_heatmap_specification, shared_unity_transformer, size_array);
}
