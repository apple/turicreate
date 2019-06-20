/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE capi_functions

#include <vector>

#include <boost/test/unit_test.hpp>
#include <capi/TuriCreate.h>
#include <core/util/test_macros.hpp>

#include "capi_utils.hpp"


BOOST_AUTO_TEST_CASE(test_function_call) {
  tc_error* error = nullptr;
  CAPI_CHECK_ERROR(error);

  // Create raw inputs to the manhattan distance function.
  double arr_x[] = { 1.0, 2.0 };
  double arr_y[] = { 5.0, 5.0 };
  tc_flexible_type* ft_x = tc_ft_create_from_double_array(arr_x, 2, &error);
  CAPI_CHECK_ERROR(error);
  tc_flexible_type* ft_y = tc_ft_create_from_double_array(arr_y, 2, &error);
  CAPI_CHECK_ERROR(error);

  // Package the inputs into a tc_parameters.
  tc_parameters* arguments = tc_parameters_create_empty(&error);
  CAPI_CHECK_ERROR(error);
  tc_parameters_add_flexible_type(arguments, "x", ft_x, &error);
  CAPI_CHECK_ERROR(error);
  tc_parameters_add_flexible_type(arguments, "y", ft_y, &error);
  CAPI_CHECK_ERROR(error);

  // Invoke the function by name.
  tc_variant* result =
      tc_function_call("_distances.manhattan", arguments, &error);

  if(error == nullptr) {
    // The function isn't found -- it was part of the API that wasn't registered

    CAPI_CHECK_ERROR(error);
  TS_ASSERT(tc_variant_is_double(result));

  // Verify the result.
  double double_result = tc_variant_double(result, &error);
    CAPI_CHECK_ERROR(error);
  TS_ASSERT(double_result == 7.0);

  // Cleanup.
    tc_release(result);
  }

  tc_release(arguments);
  tc_release(ft_x);
  tc_release(ft_y);
}

BOOST_AUTO_TEST_CASE(test_function_call_with_bad_name) {
  tc_error* error = nullptr;
  CAPI_CHECK_ERROR(error);

  tc_parameters* arguments = tc_parameters_create_empty(&error);
  CAPI_CHECK_ERROR(error);

  tc_variant* result =
      tc_function_call("b0gus 5unct10n nam3", arguments, &error);
  TS_ASSERT(error != nullptr);
  TS_ASSERT(result == nullptr);

  tc_release(error);
  tc_release(arguments);
}
