/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE vega_example_tests
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include "base_fixture.hpp"

using namespace vega_renderer::test_utils;

struct fixture : private base_fixture {

public:
  fixture() { /* setup */ }
  ~fixture() { /* teardown */ }
  void run_test_case(const std::string& specName) {
    this->run_test_case_with_path("vega/" + specName);
  }
};

BOOST_FIXTURE_TEST_SUITE(vega_example_tests, fixture)

BOOST_AUTO_TEST_CASE(testBarChart) {
  this->run_test_case("bar_chart.vg.json");
}

BOOST_AUTO_TEST_CASE(testStackedBarChart) {
  this->run_test_case("stacked_bar_chart.vg.json");
}

BOOST_AUTO_TEST_CASE(testGroupedBarChart) {
  this->run_test_case("grouped_bar_chart.vg.json");
}

BOOST_AUTO_TEST_CASE(testNestedBarChart) {
  this->run_test_case("nested_bar_chart.vg.json");
}

BOOST_AUTO_TEST_CASE(testPopulationPyramid) {
  this->run_test_case("population_pyramid.vg.json");
}

BOOST_AUTO_TEST_CASE(testLineChart) {
  this->run_test_case("line_chart.vg.json");
}

BOOST_AUTO_TEST_CASE(testAreaChart) {
  this->run_test_case("area_chart.vg.json");
}

BOOST_AUTO_TEST_CASE(testStackedAreaChart) {
  this->run_test_case("stacked_area_chart.vg.json");
}

BOOST_AUTO_TEST_CASE(testHorizonGraph) {
  this->run_test_case("horizon_graph.vg.json");
}

BOOST_AUTO_TEST_CASE(testJobVoyager) {
  this->run_test_case("job_voyager.vg.json");
}

BOOST_AUTO_TEST_CASE(testPieChart) {
  this->run_test_case("pie_chart.vg.json");
}

BOOST_AUTO_TEST_CASE(testDonutChart) {
  this->run_test_case("donut_chart.vg.json");
}

BOOST_AUTO_TEST_CASE(testRadialPlot) {
  this->run_test_case("radial_plot.vg.json");
}

BOOST_AUTO_TEST_CASE(testScatterPlot) {
  this->run_test_case("scatter_plot.vg.json");
}

BOOST_AUTO_TEST_CASE(testScatterPlotNullValues) {
  this->run_test_case("scatter_plot_null_values.vg.json");
}

BOOST_AUTO_TEST_CASE(testConnectedScatterPlot) {
  this->run_test_case("connected_scatter_plot.vg.json");
}

BOOST_AUTO_TEST_CASE(testErrorBars) {
  this->run_test_case("error_bars.vg.json");
}

BOOST_AUTO_TEST_CASE(testBarleyTrellisPlot) {
  this->run_test_case("barley_trellis_plot.vg.json");
}

BOOST_AUTO_TEST_SUITE_END()
