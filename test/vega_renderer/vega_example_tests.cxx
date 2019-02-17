/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE vega_example_tests
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include "base_fixture.hpp"

#include "examples/vega/bar_chart.vg.h"
#include "examples/vega/stacked_bar_chart.vg.h"
#include "examples/vega/grouped_bar_chart.vg.h"
#include "examples/vega/nested_bar_chart.vg.h"
#include "examples/vega/population_pyramid.vg.h"
#include "examples/vega/line_chart.vg.h"
#include "examples/vega/area_chart.vg.h"
#include "examples/vega/stacked_area_chart.vg.h"
#include "examples/vega/horizon_graph.vg.h"
#include "examples/vega/job_voyager.vg.h"
#include "examples/vega/pie_chart.vg.h"
#include "examples/vega/donut_chart.vg.h"
#include "examples/vega/radial_plot.vg.h"
#include "examples/vega/scatter_plot.vg.h"
#include "examples/vega/scatter_plot_null_values.vg.h"
#include "examples/vega/connected_scatter_plot.vg.h"
#include "examples/vega/error_bars.vg.h"
#include "examples/vega/barley_trellis_plot.vg.h"

using namespace vega_renderer::test_utils;

struct fixture : private base_fixture {

public:
  fixture() { /* setup */ }
  ~fixture() { /* teardown */ }

  void run_test_spec(unsigned char *spec, size_t len,
                              const std::string& name) {
    this->run_test_case_with_spec(this->make_format_string(spec, len), name);
  }
};

BOOST_FIXTURE_TEST_SUITE(vega_example_tests, fixture)

BOOST_AUTO_TEST_CASE(testBarChart) {
  this->run_test_spec(examples_vega_bar_chart_vg_json,
                      examples_vega_bar_chart_vg_json_len,
                      "bar_chart");
}

BOOST_AUTO_TEST_CASE(testStackedBarChart) {
  this->run_test_spec(examples_vega_stacked_bar_chart_vg_json,
                      examples_vega_stacked_bar_chart_vg_json_len,
                      "stacked_bar_chart");
}

BOOST_AUTO_TEST_CASE(testGroupedBarChart) {
  this->run_test_spec(examples_vega_grouped_bar_chart_vg_json,
                      examples_vega_grouped_bar_chart_vg_json_len,
                      "grouped_bar_chart");
}

BOOST_AUTO_TEST_CASE(testNestedBarChart) {
  this->run_test_spec(examples_vega_nested_bar_chart_vg_json,
                      examples_vega_nested_bar_chart_vg_json_len,
                      "nested_bar_chart");
}

BOOST_AUTO_TEST_CASE(testPopulationPyramid) {
  this->run_test_spec(examples_vega_population_pyramid_vg_json,
                      examples_vega_population_pyramid_vg_json_len,
                      "population_pyramid");
}

BOOST_AUTO_TEST_CASE(testLineChart) {
  this->run_test_spec(examples_vega_line_chart_vg_json,
                      examples_vega_line_chart_vg_json_len,
                      "line_chart");
}

BOOST_AUTO_TEST_CASE(testAreaChart) {
  this->run_test_spec(examples_vega_area_chart_vg_json,
                      examples_vega_area_chart_vg_json_len,
                      "area_chart");
}

BOOST_AUTO_TEST_CASE(testStackedAreaChart) {
  this->run_test_spec(examples_vega_stacked_area_chart_vg_json,
                      examples_vega_stacked_area_chart_vg_json_len,
                      "stacked_area_chart");
}

BOOST_AUTO_TEST_CASE(testHorizonGraph) {
  this->run_test_spec(examples_vega_horizon_graph_vg_json,
                      examples_vega_horizon_graph_vg_json_len,
                      "horizon_graph");
}

BOOST_AUTO_TEST_CASE(testJobVoyager) {
  this->run_test_spec(examples_vega_job_voyager_vg_json,
                      examples_vega_job_voyager_vg_json_len,
                      "job_voyager");
}

BOOST_AUTO_TEST_CASE(testPieChart) {
  this->run_test_spec(examples_vega_pie_chart_vg_json,
                      examples_vega_pie_chart_vg_json_len,
                      "pie_chart");
}

BOOST_AUTO_TEST_CASE(testDonutChart) {
  this->run_test_spec(examples_vega_donut_chart_vg_json,
                      examples_vega_donut_chart_vg_json_len,
                      "donut_chart");
}

BOOST_AUTO_TEST_CASE(testRadialPlot) {
  this->run_test_spec(examples_vega_radial_plot_vg_json,
                      examples_vega_radial_plot_vg_json_len,
                      "radial_plot");
}

BOOST_AUTO_TEST_CASE(testScatterPlot) {
  this->run_test_spec(examples_vega_scatter_plot_vg_json,
                      examples_vega_scatter_plot_vg_json_len,
                      "scatter_plot");
}

BOOST_AUTO_TEST_CASE(testScatterPlotNullValues) {
  this->run_test_spec(examples_vega_scatter_plot_null_values_vg_json,
                      examples_vega_scatter_plot_null_values_vg_json_len,
                      "scatter_plot_null_values");
}

BOOST_AUTO_TEST_CASE(testConnectedScatterPlot) {
  this->run_test_spec(examples_vega_connected_scatter_plot_vg_json,
                      examples_vega_connected_scatter_plot_vg_json_len,
                      "connected_scatter_plot");
}

BOOST_AUTO_TEST_CASE(testErrorBars) {
  this->run_test_spec(examples_vega_error_bars_vg_json,
                      examples_vega_error_bars_vg_json_len,
                      "error_bars");
}

BOOST_AUTO_TEST_CASE(testBarleyTrellisPlot) {
  this->run_test_spec(examples_vega_barley_trellis_plot_vg_json,
                      examples_vega_barley_trellis_plot_vg_json_len,
                      "barley_trellis_plot");
}

BOOST_AUTO_TEST_SUITE_END()
