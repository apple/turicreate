/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <vector>
#include <cmath>

#include <iostream>
#include <logger/logger.hpp>
#include <logger/assertions.hpp>
#include <util/bitops.hpp>
#include <util/cityhash_tc.hpp>
#include <util/fast_integer_power.hpp>

using namespace turi;
using namespace std;

struct fast_power_test  {
 public:
  void _run_test(double v, const std::vector<size_t>& powers) {

    fast_integer_power vp(v);

    for(size_t n : powers) {
      double v_ref = std::pow(v, n);
      double v_check = vp.pow(n);

      if(abs(v_ref - v_check) / (1.0 + ceil(v_ref + v_check) ) > 1e-8 ) {
        std::ostringstream ss;
        ss << "Wrong value: "
           << v << " ^ " << n << " = " << v_ref
           << "; retrieved = " << v_check << std::endl;

        ASSERT_MSG(false, ss.str().c_str());
      }
    }
  }

  void test_low_powers() {
    _run_test(0.75, {0, 1, 2, 3, 4, 5, 6, 7, 8});
  }

  void test_lots_of_powers() {
    std::vector<size_t> v(5000);

    for(size_t i = 0; i < 5000; ++i)
      v[i] = i;

    _run_test(0.99, v);
    _run_test(1.02, v);
  }

  void test_many_random() {
    std::vector<size_t> v(50000);

    for(size_t i = 0; i < 50000; ++i)
      v[i] = hash64(i);

    _run_test(1 - 1e-6, v);
    _run_test(1 + 1e-6, v);
  }
};

BOOST_FIXTURE_TEST_SUITE(_fast_power_test, fast_power_test)
BOOST_AUTO_TEST_CASE(test_low_powers) {
  fast_power_test::test_low_powers();
}
BOOST_AUTO_TEST_CASE(test_lots_of_powers) {
  fast_power_test::test_lots_of_powers();
}
BOOST_AUTO_TEST_CASE(test_many_random) {
  fast_power_test::test_many_random();
}
BOOST_AUTO_TEST_SUITE_END()
