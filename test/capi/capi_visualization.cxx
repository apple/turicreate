/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <capi/TuriCreate.h>
#include "capi_utils.hpp"

using namespace turi;

/*
 * Test streaming visualization C API
 * High level goal: ensure the same output between the C API
 *                  and the equivalent C++ Plot object method(s).
 * Non-goal (for this test): ensure that the resulting values are correct.
 * C++ unit tests should cover the correctness of the results.
 */
class capi_test_visualization {
    private:
        tc_sarray* m_sa_uniform;

    public:
        capi_test_visualization() {
            tc_error* error = NULL;

            std::vector<double> v = {0,1,2,3,4,5};
            tc_flex_list* fl = make_flex_list_double(v);
            m_sa_uniform = tc_sarray_create_from_list(fl, &error);
            CAPI_CHECK_ERROR(error);
        }

        ~capi_test_visualization() {
            tc_release(m_sa_uniform);
        }

        void test_1d_plots() {
            //const auto& expected = m_sa_uniform->value.plot("", "Title", "X Axis Title", "Y Axis Title");

        }
};

BOOST_FIXTURE_TEST_SUITE(_capi_test_visualization, capi_test_visualization)
BOOST_AUTO_TEST_CASE(test_1d_plots) {
  capi_test_visualization::test_1d_plots();
}
BOOST_AUTO_TEST_SUITE_END()