/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "heatmap.hpp"

#include "process_wrapper.hpp"
#include "thread.hpp"
#include "vega_data.hpp"
#include "vega_spec.hpp"

#include <core/parallel/lambda_omp.hpp>
#include <visualization/server/batch_size.hpp>
#include <visualization/server/transformation.hpp>

#include <cmath>
#include <thread>

using namespace turi;
using namespace turi::visualization;

constexpr static size_t NUM_BINS = 60;
static const std::string x_name = "x";
static const std::string y_name = "y";

static inline gl_sarray validate_dtype(const gl_sarray& input) {
  flex_type_enum dtype = input.dtype();
  if (dtype != flex_type_enum::INTEGER &&
      dtype != flex_type_enum::FLOAT) {
    log_and_throw("dtype of the provided SArray is not valid for heatmap. heatmap can only operate on INTEGER or FLOAT SArrays.");
  }
  return input;
}

std::shared_ptr<Plot> turi::visualization::plot_heatmap(
                                       const gl_sarray& x,
                                       const gl_sarray& y,
                                       const flexible_type& xlabel,
                                       const flexible_type& ylabel,
                                       const flexible_type& title) {
  validate_dtype(x);
  validate_dtype(y);

  std::stringstream ss;
  ss << heatmap_spec(xlabel, ylabel, title);
  std::string heatmap_specification = ss.str();

  double size_array = static_cast<double>(x.size());

  heatmap hm;

  gl_sframe temp_sf;

  temp_sf[x_name] = x;
  temp_sf[y_name] = y;

  hm.init(temp_sf, batch_size(x, y));

  std::shared_ptr<transformation_base> shared_unity_transformer = std::make_shared<heatmap>(hm);
  return std::make_shared<Plot>(heatmap_specification, shared_unity_transformer, size_array);
}

heatmap_result::heatmap_result() : bins(NUM_BINS, std::vector<flex_int>(NUM_BINS, 0)) {}

void heatmap_result::init(double xMin, double xMax, double yMin, double yMax) {
  // make sure extrema are initialized
  extrema.x.update(xMin);
  extrema.x.update(xMax);
  extrema.y.update(yMin);
  extrema.y.update(yMax);
}

void heatmap::init(const gl_sframe& source, size_t batch_size) {
  // initialize parent class
  groupby<heatmap_result>::init(source, batch_size);

  // initialize heatmap_result
  const auto& head = source.head(10000); // infer min/max from first 10k rows
  const auto& x = head[x_name];
  const auto& y = head[y_name];
  m_transformer->init(x.min(), x.max(), y.min(), y.max());
}

std::vector<heatmap_result> heatmap::split_input(size_t num_threads) {
  // preserve extrema when returning new aggregators
  auto ret = groupby<heatmap_result>::split_input(num_threads);
  for (auto& result : ret) {
    result.init(m_transformer->extrema.x.get_min(),
                m_transformer->extrema.x.get_max(),
                m_transformer->extrema.y.get_min(),
                m_transformer->extrema.y.get_max());
  }
  return ret;
}

static inline size_t get_bin_idx(
  flex_float value,
  flex_float min,
  flex_float max
) {
  size_t ret = static_cast<size_t>(
    std::floor(
      (static_cast<double>(value - min) /
       static_cast<double>(max - min)) *
      static_cast<double>(NUM_BINS)
    )
  );
  if (ret == NUM_BINS) {
    ret--;
  }
  DASSERT_LT(ret, NUM_BINS);
  return ret;
}

group_aggregate_value* heatmap_result::new_instance() const {
  heatmap_result* ret = new heatmap_result;
  ret->extrema = extrema; // initialize with the same extrema
  return ret;
}

void heatmap_result::widen_x(double value) {
  while (value < extrema.x.get_min() || value > extrema.x.get_max()) {
    // since we are row-major, for each two X values, we must combine the Y counts (preserving Y)

    // first, combine bins next to each other (every other bin)
    for (ssize_t i=(NUM_BINS / 2) - 1; i>0; i-=2) {
      for (size_t j=0; j<NUM_BINS; j++) {
        bins[i][j] += bins[i-1][j];
      }
    }
    for (size_t i=(NUM_BINS / 2); i<NUM_BINS; i+=2) {
      for (size_t j=0; j<NUM_BINS; j++) {
        bins[i][j] += bins[i+1][j];
      }
    }

    // then, collapse them inward towards the center
    for (size_t i=0; i<(NUM_BINS/4); i++) {
      bins[(NUM_BINS/2) + i] = bins[(NUM_BINS/2) + (2 * i)];
      bins[(NUM_BINS/2) - (i + 1)] = bins[(NUM_BINS/2) - ((2 * i) + 1)];
    }

    // finally, zero out the newly-unused bins
    for (size_t i=((NUM_BINS * 3) / 4); i<NUM_BINS; i++) {
      bins[i] = std::vector<flex_int>(NUM_BINS, 0);
    }
    for (size_t i=0; i<(NUM_BINS/4); i++) {
      bins[i] = std::vector<flex_int>(NUM_BINS, 0);
    }

    // double the range of X axis
    flex_float cur_min = extrema.x.get_min();
    flex_float cur_max = extrema.x.get_max();
    flex_float range = cur_max - cur_min;
    DASSERT_GT(range, 0); // can't have a heatmap with no range
    flex_float new_min = cur_min - (0.5 * range);
    flex_float new_max = cur_max + (0.5 * range);
    extrema.x.update(new_min);
    extrema.x.update(new_max);
  }
}

void heatmap_result::widen_y(double value) {
  while (value < extrema.y.get_min() || value > extrema.y.get_max()) {
    // since we are row-major, for each two Y values, we must combine the counts (ignoring X)

    for (size_t i=0; i<NUM_BINS; i++) {
      // first, combine bins next to each other (every other bin)
      for (ssize_t j=(NUM_BINS / 2) - 1; j>0; j-=2) {
        bins[i][j] += bins[i][j-1];
      }

      for (size_t j=(NUM_BINS / 2); j<NUM_BINS; j+=2) {
        bins[i][j] += bins[i][j+1];
      }

      // then, collapse them inward towards the center
      for (size_t j=0; j<(NUM_BINS/4); j++) {
        bins[i][(NUM_BINS/2) + j] = bins[i][(NUM_BINS/2) + (2 * j)];
        bins[i][(NUM_BINS/2) - (j + 1)] = bins[i][(NUM_BINS/2) - ((2 * j) + 1)];
      }

      // finally, zero out the newly-unused bins
      for (size_t j=((NUM_BINS * 3) / 4); j<NUM_BINS; j++) {
        bins[i][j] = 0;
      }
      for (size_t j=0; j<(NUM_BINS/4); j++) {
        bins[i][j] = 0;
      }
    }

    // double the range of Y axis
    flex_float cur_min = extrema.y.get_min();
    flex_float cur_max = extrema.y.get_max();
    flex_float range = cur_max - cur_min;
    DASSERT_GT(range, 0); // can't have a heatmap with no range
    flex_float new_min = cur_min - (0.5 * range);
    flex_float new_max = cur_max + (0.5 * range);
    extrema.y.update(new_min);
    extrema.y.update(new_max);
  }
}

void heatmap_result::add_element_simple(const flexible_type& flex) {
  // expect [x,y] input as float (in flex_list form).
  flex_list asList = flex.get<flex_list>();
  DASSERT_EQ(asList.size(), 2);
  flexible_type xFlex = asList[0];
  flexible_type yFlex = asList[1];
  flex_float x, y;
  switch (xFlex.get_type()) {
    case flex_type_enum::FLOAT:
      x = xFlex.get<flex_float>();
      break;
    case flex_type_enum::INTEGER:
      x = xFlex.get<flex_int>();
      break;
    default:
      throw std::runtime_error("Expected X axis to be int or float in heatmap.");
  }
  switch (yFlex.get_type()) {
    case flex_type_enum::FLOAT:
      y = yFlex.get<flex_float>();
      break;
    case flex_type_enum::INTEGER:
      y = yFlex.get<flex_int>();
      break;
    default:
      throw std::runtime_error("Expected X axis to be int or float in heatmap.");
  }

  // first, check extrema, resize if needed
  widen_x(x);
  widen_y(y);

  // now that the value will fit, determine bin index on both axes
  size_t x_idx = get_bin_idx(x, extrema.x.get_min(), extrema.x.get_max());
  size_t y_idx = get_bin_idx(y, extrema.y.get_min(), extrema.y.get_max());

  // and finally increment the count
  bins[x_idx][y_idx]++;
}

void heatmap_result::combine(const group_aggregate_value& generic_other) {
  heatmap_result other(dynamic_cast<const heatmap_result&>(generic_other));

  // find common min/max for both
  double new_x_min = std::min(extrema.x.get_min(), other.extrema.x.get_min());
  double new_x_max = std::max(extrema.x.get_max(), other.extrema.x.get_max());
  double new_y_min = std::min(extrema.y.get_min(), other.extrema.y.get_min());
  double new_y_max = std::max(extrema.y.get_max(), other.extrema.y.get_max());


  // widen self to accommodate other (if needed)
  widen_x(new_x_min);
  widen_x(new_x_max);
  widen_y(new_y_min);
  widen_y(new_y_max);

  // widen other to accommodate self (if needed)
  other.widen_x(new_x_min);
  other.widen_x(new_x_max);
  other.widen_y(new_y_min);
  other.widen_y(new_y_max);

  // self and other should now have equal extrema
  // (thus their bin counts can be simply added)
  DASSERT_EQ(extrema, other.extrema);
  for (size_t i=0; i<NUM_BINS; i++) {
    for (size_t j=0; j<NUM_BINS; j++) {
      bins[i][j] += other.bins[i][j];
    }
  }
}

flexible_type heatmap_result::emit() const {
  flex_list ret;

  size_t bins_size = bins.size();

  for (size_t i=0; i<bins_size; i++) {
    // row

    const auto& row = bins[i];
    size_t row_size = row.size();
    double xScale = static_cast<double>(i) / static_cast<double>(bins_size);
    double xWidth = extrema.x.get_max() - extrema.x.get_min();
    double xBinWidth = xWidth / static_cast<double>(NUM_BINS);
    double yWidth = extrema.y.get_max() - extrema.y.get_min();
    double yBinWidth = yWidth / static_cast<double>(NUM_BINS);
    double x1 = static_cast<double>(extrema.x.get_min()) + (xScale * xWidth);
    double x2 = x1 + xBinWidth;

    for (size_t j=0; j<row_size; j++) {
      // col

      flex_dict value;
      size_t count = row[j];
      double yScale = static_cast<double>(j) / static_cast<double>(row_size);
      double y1 = (yScale * yWidth) + extrema.y.get_min();
      double y2 = y1 + yBinWidth;
      value.push_back(std::make_pair("x_left", x1));
      value.push_back(std::make_pair("x_right", x2));
      value.push_back(std::make_pair("y_left", y1));
      value.push_back(std::make_pair("y_right", y2));
      value.push_back(std::make_pair("count", count));
      ret.push_back(value);
    }
  }

  return ret;
}

bool heatmap_result::support_type(flex_type_enum type) const {
  return type == flex_type_enum::LIST;
}

std::string heatmap_result::name() const {
  return "2d Heatmap";
}

void heatmap_result::save(oarchive& oarc) const {
  throw std::runtime_error("save not supported for heatmap result");
}

void heatmap_result::load(iarchive& iarc) {
  throw std::runtime_error("load not supported for heatmap result");
}

std::string heatmap_result::vega_column_data(bool) const {
  std::stringstream ss;
  flexible_type flex_data = this->emit();
  flex_list data = flex_data.get<flex_list>();
  for (size_t i=0; i<data.size(); i++) {
    ss << data[i];
    if (i != data.size() - 1) {
      ss << ",";
    }
  }
  return ss.str();
}
