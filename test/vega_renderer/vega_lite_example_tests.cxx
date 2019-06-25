/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE vega_example_tests
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include "base_fixture.hpp"

using namespace vega_renderer::test_utils;

#include "examples/vega_lite/simple_bar_chart.vl.h"
#include "examples/vega_lite/histogram.vl.h"
#include "examples/vega_lite/aggregate_bar_chart.vl.h"
#include "examples/vega_lite/grouped_bar_chart.vl.h"
#include "examples/vega_lite/gantt_chart.vl.h"
#include "examples/vega_lite/layered_bar_chart.vl.h"
#include "examples/vega_lite/isotype_bar_chart.vl.h"
#include "examples/vega_lite/isotype_bar_chart_with_emoji.vl.h"
#include "examples/vega_lite/bubble_plot_natural_disasters.vl.h"
#include "examples/vega_lite/scatterplot.vl.h"
#include "examples/vega_lite/line_chart_with_stroked_point_mark.vl.h"

struct fixture : private base_fixture {

public:
  fixture() { /* setup */ }
  ~fixture() { /* teardown */ }

  void run_test_spec(unsigned char *spec, size_t len,
                              const std::string& name) {
    this->run_test_case_with_spec(this->make_format_string(spec, len), name);
  }

  void run_test_spec(unsigned char *spec, size_t len,
                    const std::string& name, double acceptableDiff) {
    this->run_test_case_with_spec(this->make_format_string(spec, len), name, acceptableDiff);
  }
};

BOOST_FIXTURE_TEST_SUITE(vega_lite_example_tests, fixture)

BOOST_AUTO_TEST_CASE(testSimpleBarChart) {
  this->run_test_spec(examples_vega_lite_simple_bar_chart_vl_json,
                      examples_vega_lite_simple_bar_chart_vl_json_len,
                      "simple_bar_chart");
}

BOOST_AUTO_TEST_CASE(testHistogram) {
  this->run_test_spec(examples_vega_lite_histogram_vl_json,
                      examples_vega_lite_histogram_vl_json_len,
                      "histogram");
}

BOOST_AUTO_TEST_CASE(testAggregateBarChart) {
  this->run_test_spec(examples_vega_lite_aggregate_bar_chart_vl_json,
                      examples_vega_lite_aggregate_bar_chart_vl_json_len,
                      "aggregate_bar_chart");
}

BOOST_AUTO_TEST_CASE(testGroupedBarChart) {
  this->run_test_spec(examples_vega_lite_grouped_bar_chart_vl_json,
                      examples_vega_lite_grouped_bar_chart_vl_json_len,
                      "grouped_bar_chart");
}

BOOST_AUTO_TEST_CASE(testGanttChart) {
  this->run_test_spec(examples_vega_lite_gantt_chart_vl_json,
                      examples_vega_lite_gantt_chart_vl_json_len,
                      "gantt_chart");
}

BOOST_AUTO_TEST_CASE(testLayeredBarChart) {
  this->run_test_spec(examples_vega_lite_layered_bar_chart_vl_json,
                      examples_vega_lite_layered_bar_chart_vl_json_len,
                      "layered_bar_chart");
}

BOOST_AUTO_TEST_CASE(testIsoTypeBarChart) {
  this->run_test_spec(examples_vega_lite_isotype_bar_chart_vl_json,
                      examples_vega_lite_isotype_bar_chart_vl_json_len,
                      "isotype_bar_chart");
}

BOOST_AUTO_TEST_CASE(testIsoTypeBarChartWithEmoji) {
  this->run_test_spec(examples_vega_lite_isotype_bar_chart_with_emoji_vl_json,
                      examples_vega_lite_isotype_bar_chart_with_emoji_vl_json_len,
                      "isotype_bar_chart_with_emoji",
                      3.19);
}

BOOST_AUTO_TEST_CASE(testBubblePlotNaturalDisasters) {
  this->run_test_spec(examples_vega_lite_bubble_plot_natural_disasters_vl_json,
                      examples_vega_lite_bubble_plot_natural_disasters_vl_json_len,
                      "bubble_plot_natural_disasters");
}

BOOST_AUTO_TEST_CASE(testScatterPlot) {
  this->run_test_spec(examples_vega_lite_scatterplot_vl_json,
                      examples_vega_lite_scatterplot_vl_json_len,
                      "scatterplot");
}

BOOST_AUTO_TEST_CASE(testLineChartWithStrokedPointMark) {
  this->run_test_spec(examples_vega_lite_line_chart_with_stroked_point_mark_vl_json,
                      examples_vega_lite_line_chart_with_stroked_point_mark_vl_json_len,
                      "line_chart_with_stroked_point_mark");
}

BOOST_AUTO_TEST_SUITE_END()
