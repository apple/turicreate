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
    this->run_test_case_with_path("vega-lite/" + specName);
  }
  void run_test_case(const std::string& specName, double acceptableDiff) {
    this->run_test_case_with_path("vega-lite/" + specName, acceptableDiff);
  }
};

BOOST_FIXTURE_TEST_SUITE(vega_lite_example_tests, fixture)

BOOST_AUTO_TEST_CASE(testSimpleBarChart) {
  this->run_test_case("simple_bar_chart.vl.json");
}

BOOST_AUTO_TEST_CASE(testHistogram) {
  this->run_test_case("histogram.vl.json");
}

BOOST_AUTO_TEST_CASE(testAggregateBarChart) {
  this->run_test_case("aggregate_bar_chart.vl.json");
}

BOOST_AUTO_TEST_CASE(testGroupedBarChart) {
  this->run_test_case("grouped_bar_chart.vl.json");
}

BOOST_AUTO_TEST_CASE(testGanttChart) {
  this->run_test_case("gantt_chart.vl.json");
}

BOOST_AUTO_TEST_CASE(testLayeredBarChart) {
  this->run_test_case("layered_bar_chart.vl.json");
}

BOOST_AUTO_TEST_CASE(testIsoTypeBarChart) {
  this->run_test_case("isotype_bar_chart.vl.json");
}

BOOST_AUTO_TEST_CASE(testIsoTypeBarChartWithEmoji) {
  this->run_test_case("isotype_bar_chart_with_emoji.vl.json", 3.19);
}

BOOST_AUTO_TEST_CASE(testBubblePlotNaturalDisasters) {
  this->run_test_case("bubble_plot_natural_disasters.vl.json");
}

BOOST_AUTO_TEST_CASE(testScatterPlot) {
  this->run_test_case("scatterplot.vl.json");
}

BOOST_AUTO_TEST_CASE(testLineChartWithStrokedPointMark) {
  this->run_test_case("line_chart_with_stroked_point_mark.vl.json");
}

BOOST_AUTO_TEST_SUITE_END()
