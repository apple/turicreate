/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE vega_js_unit_tests
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include "vega_js_test_fixture.hpp"
#include "vega_js_tests/point_in_path_tests.h"

using namespace vega_renderer::test_utils;

struct fixture : private vega_js_test_fixture {

public:
  fixture() { /* setup */ }
  ~fixture() { /* teardown */ }

  void run_test_js(unsigned char *spec, size_t len,
                              const std::string& name) {
    this->run_test_case_with_js(this->make_format_string(spec, len), name);
  }
};

BOOST_FIXTURE_TEST_SUITE(vega_js_unit_tests, fixture)

BOOST_AUTO_TEST_CASE(pointInPathTests) {
  this->run_test_js(vega_js_tests_point_in_path_tests_js,
                      vega_js_tests_point_in_path_tests_js_len,
                      "point_in_path_tests");
}

BOOST_AUTO_TEST_SUITE_END()