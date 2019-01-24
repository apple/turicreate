/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <sframe/sframe_random_access.hpp>
#include <unity/toolkits/util/random_sframe_generation.hpp>
#include <unity/toolkits/util/sframe_test_util.hpp>
#include <util/basic_types.hpp>
#include <util/string_util.hpp>
#include <util/test_macros.hpp>
#include <util/time_spec.hpp>

using namespace turi;

struct sframe_random_access_test {
  void test_sframe_random_access_conversion() {
    int64_t seed = 0;

    vector<int64_t> n_rows_pool = {
      10,
      1000,
      100000,
    };

    vector<string> column_types_pool = {
      "z",
      "n",
      "r",
      "R",
      "S",
      "X",
      "H",
      "znr",
      "rHnS",
      "zznHrRSXH",
    };

    for (int64_t n_rows : n_rows_pool) {
      for (string column_types : column_types_pool) {
        auto sf1 = _generate_random_sframe(
          n_rows, column_types, seed, false, 0.0);

        auto t0 = now();
        auto sfr = sframe_random_access::from_sframe(sf1);
        auto sf2 = sframe_random_access::to_sframe(sfr);
        auto tr = (now() - t0).val();

        TS_ASSERT(check_equality_gl_sframe(sf1, sf2));
        fmt(cerr,
            "test_sframe_random_access_conversion complete [%v sec]: %v, %v\n",
            cc_sprintf("%5.3f", tr / 1.0e9),
            n_rows,
            column_types);

        ++seed;
      }
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_sframe_random_access_test, sframe_random_access_test)
BOOST_AUTO_TEST_CASE(test_sframe_random_access_conversion) {
  sframe_random_access_test::test_sframe_random_access_conversion();
}
BOOST_AUTO_TEST_SUITE_END()
