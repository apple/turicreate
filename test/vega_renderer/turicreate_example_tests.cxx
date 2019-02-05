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
    this->run_test_case_with_path("turicreate/" + specName);
  }
};

BOOST_FIXTURE_TEST_SUITE(turicreate_example_tests, fixture)

BOOST_AUTO_TEST_CASE(testCategoricalHeatmap) {
  this->run_test_case("mushroom_categorical_heatmap.vg.json");
}

BOOST_AUTO_TEST_CASE(testSFrameSummary) {
  this->run_test_case("mushroom_sframe_summary.vg.json");
}

BOOST_AUTO_TEST_CASE(testHeatmap) {
  this->run_test_case("clang_format_heatmap.vg.json");
}

BOOST_AUTO_TEST_CASE(testScatterPlot) {
  this->run_test_case("clang_format_scatterplot.vg.json");
}

BOOST_AUTO_TEST_CASE(testBoxesAndWhiskers) {
  this->run_test_case("clang_format_boxes_and_whiskers.vg.json");
}

BOOST_AUTO_TEST_CASE(testHistogram) {
  this->run_test_case("clang_format_histogram.vg.json");
}

BOOST_AUTO_TEST_CASE(testCategoricalHistogram) {
  this->run_test_case("mushroom_categorical_histogram.vg.json");
}

BOOST_AUTO_TEST_SUITE_END()
