#define BOOST_TEST_MODULE capi_functions

#include <vector>

#include <boost/test/unit_test.hpp>
#include <capi/TuriCore.h>
#include <util/test_macros.hpp>

#include "capi_utils.hpp"

namespace {
  
}  // namespace

BOOST_AUTO_TEST_CASE(test_function_call) {
  tc_error* error = nullptr;
  tc_initialize("/tmp/", &error);
  TS_ASSERT(error == NULL);

  // Create raw inputs to the manhattan distance function.
  double arr_x[] = { 1.0, 2.0 };
  double arr_y[] = { 5.0, 5.0 };
  tc_flexible_type* ft_x = tc_ft_create_from_double_array(arr_x, 2, &error);
  TS_ASSERT(error == nullptr);
  tc_flexible_type* ft_y = tc_ft_create_from_double_array(arr_y, 2, &error);
  TS_ASSERT(error == nullptr);

  // Package the inputs into a tc_parameters.
  tc_parameters* arguments = tc_parameters_create_empty(&error);
  TS_ASSERT(error == nullptr);
  tc_parameters_add_flexible_type(arguments, "x", ft_x, &error);
  TS_ASSERT(error == nullptr);
  tc_parameters_add_flexible_type(arguments, "y", ft_y, &error);
  TS_ASSERT(error == nullptr);

  // Invoke the function by name.
  tc_variant* result =
      tc_function_call("_distances.manhattan", arguments, &error);
  TS_ASSERT(error == nullptr);
  TS_ASSERT(tc_variant_is_double(result));

  // Verify the result.
  double double_result = tc_variant_double(result, &error);
  TS_ASSERT(error == nullptr);
  TS_ASSERT(double_result == 7.0);

  // Cleanup.
  tc_variant_destroy(result);
  tc_parameters_destroy(arguments);
  tc_ft_destroy(ft_x);
  tc_ft_destroy(ft_y);
}

BOOST_AUTO_TEST_CASE(test_function_call_with_bad_name) {
  tc_error* error = nullptr;
  tc_initialize("/tmp/", &error);
  TS_ASSERT(error == NULL);

  tc_parameters* arguments = tc_parameters_create_empty(&error);
  TS_ASSERT(error == nullptr);

  tc_variant* result =
      tc_function_call("b0gus 5unct10n nam3", arguments, &error);
  TS_ASSERT(error != nullptr);
  TS_ASSERT(result == nullptr);

  tc_error_destroy(&error);
  tc_parameters_destroy(arguments);
}
