/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE vega_example_tests
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include "base_fixture.hpp"

#include "examples/turicreate/mushroom_categorical_heatmap.vg.h"
#include "examples/turicreate/mushroom_sframe_summary.vg.h"
#include "examples/turicreate/clang_format_heatmap.vg.h"
#include "examples/turicreate/clang_format_scatterplot.vg.h"
#include "examples/turicreate/clang_format_boxes_and_whiskers.vg.h"
#include "examples/turicreate/clang_format_histogram.vg.h"
#include "examples/turicreate/mushroom_categorical_histogram.vg.h"

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

BOOST_FIXTURE_TEST_SUITE(turicreate_example_tests, fixture)

BOOST_AUTO_TEST_CASE(testCategoricalHeatmap) {
  this->run_test_spec(examples_turicreate_mushroom_categorical_heatmap_vg_json,
                      examples_turicreate_mushroom_categorical_heatmap_vg_json_len,
                      "mushroom_categorical_heatmap");
}

BOOST_AUTO_TEST_CASE(testSFrameSummary) {
  this->run_test_spec(examples_turicreate_mushroom_sframe_summary_vg_json,
                      examples_turicreate_mushroom_sframe_summary_vg_json_len,
                      "mushroom_sframe_summary");
}

BOOST_AUTO_TEST_CASE(testHeatmap) {
  this->run_test_spec(examples_turicreate_clang_format_heatmap_vg_json,
                      examples_turicreate_clang_format_heatmap_vg_json_len,
                      "clang_format_heatmap");
}

BOOST_AUTO_TEST_CASE(testScatterPlot) {
  this->run_test_spec(examples_turicreate_clang_format_scatterplot_vg_json,
                      examples_turicreate_clang_format_scatterplot_vg_json_len,
                      "clang_format_scatterplot");
}

BOOST_AUTO_TEST_CASE(testBoxesAndWhiskers) {
  this->run_test_spec(examples_turicreate_clang_format_boxes_and_whiskers_vg_json,
                      examples_turicreate_clang_format_boxes_and_whiskers_vg_json_len,
                      "clang_format_boxes_and_whiskers");
}

BOOST_AUTO_TEST_CASE(testHistogram) {
  this->run_test_spec(examples_turicreate_clang_format_histogram_vg_json,
                      examples_turicreate_clang_format_histogram_vg_json_len,
                      "clang_format_histogram");
}

BOOST_AUTO_TEST_CASE(testCategoricalHistogram) {
  this->run_test_spec(examples_turicreate_mushroom_categorical_histogram_vg_json,
                      examples_turicreate_mushroom_categorical_histogram_vg_json_len,
                      "mushroom_categorical_histogram");
}

BOOST_AUTO_TEST_SUITE_END()
