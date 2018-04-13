/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "categorical_heatmap.hpp"

#include "process_wrapper.hpp"
#include "thread.hpp"
#include "vega_data.hpp"
#include "vega_spec.hpp"

#include <parallel/lambda_omp.hpp>
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
  std::stringstream ss;
  size_t x = 0;

  auto items_list = emit().get<flex_dict>();
  size_t size_list = items_list.size();
  for (size_t i=0; i<size_list; i++) {

    const std::pair<flexible_type, flexible_type>& pair = items_list[i];
    const flex_list& values = pair.first.get<flex_list>();

    // for now, skip undefined
    if (values[0].get_type() == flex_type_enum::UNDEFINED ||
        values[1].get_type() == flex_type_enum::UNDEFINED) {
      continue;
    }

    const flex_string& xValue = values[0].get<flex_string>();
    const flex_string& yValue = values[1].get<flex_string>();
    flex_int count = pair.second.get<flex_int>();

    if(x !=  0){
      ss << ",";
    }

    ss << "{\"x\": ";
    ss << extra_label_escape(xValue);
    ss << ", \"y\": ";
    ss << extra_label_escape(yValue);
    ss << ", \"count\": ";
    ss << count;
    ss << "}";

    x++;
  }

  return ss.str();
}


std::shared_ptr<Plot> turi::visualization::plot_categorical_heatmap(const std::string& path_to_client,
                                                      const gl_sarray& x,
                                                      const gl_sarray& y,
                                                      const std::string& xlabel,
                                                      const std::string& ylabel,
                                                      const std::string& title) {

    std::stringstream ss;
    ss << categorical_heatmap_spec(xlabel, ylabel, title);
    std::string categorical_heatmap_specification = ss.str();

    double size_array = static_cast<double>(x.size());

    categorical_heatmap hm;

    gl_sframe temp_sf;

    temp_sf["x"] = x;
    temp_sf["y"] = y;

    hm.init(temp_sf);

    std::shared_ptr<transformation_base> shared_unity_transformer = std::make_shared<categorical_heatmap>(hm);
    return std::make_shared<Plot>(path_to_client, categorical_heatmap_specification, shared_unity_transformer, size_array);
}
