/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE capi_flexible_types
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#include <capi/TuriCreate.h>
#include <vector>
#include "capi_utils.hpp"

BOOST_AUTO_TEST_CASE(test_tc_ft_create_empty) {
  tc_error* error = nullptr;
  tc_flexible_type* ft = tc_ft_create_empty(&error);
  CAPI_CHECK_ERROR(error);
  TS_ASSERT(ft != nullptr);
  TS_ASSERT(tc_ft_is_undefined(ft));
  TS_ASSERT(!tc_ft_is_int64(ft));
}

BOOST_AUTO_TEST_CASE(test_flex_list_double) {
  std::vector<double> v = {1, 2, 4.5, 9, 10000000};

  tc_error* error = NULL;

  tc_flex_list* fl = tc_flex_list_create(&error);

  CAPI_CHECK_ERROR(error);

  size_t pos = 0;

  for (double t : v) {
    tc_flexible_type* ft = tc_ft_create_from_double(t, &error);

    CAPI_CHECK_ERROR(error);

    size_t idx = tc_flex_list_add_element(fl, ft, &error);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT_EQUALS(idx, pos);
    ++pos;

    size_t actual_size = tc_flex_list_size(fl);

    TS_ASSERT_EQUALS(pos, actual_size);

    tc_release(ft);
  }

  // Go through and make sure things are equal
  for (size_t i = 0; i < v.size(); ++i) {
    tc_flexible_type* ft = tc_flex_list_extract_element(fl, i, &error);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT(tc_ft_is_double(ft) != 0);

    double val = tc_ft_double(ft, &error);
    CAPI_CHECK_ERROR(error);

    TS_ASSERT(v[i] == val);

    tc_release(ft);
  }
}

