#define BOOST_TEST_MODULE capi_flexible_types
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <capi/TuriCore.h>
#include <vector>

BOOST_AUTO_TEST_CASE(test_flex_list_double) {
  std::vector<double> v = {1, 2, 4.5, 9, 10000000};

  tc_error* error = NULL;

  tc_flex_list* fl = tc_flex_list_create(&error);

  TS_ASSERT(error == NULL);

  size_t pos = 0;

  for (double t : v) {
    tc_flexible_type* ft = tc_ft_create_from_double(t, &error);

    TS_ASSERT(error == NULL);

    size_t idx = tc_flex_list_add_element(fl, ft, &error);

    TS_ASSERT(error == NULL);

    TS_ASSERT_EQUALS(idx, pos);
    ++pos;

    size_t actual_size = tc_flex_list_size(fl);

    TS_ASSERT_EQUALS(pos, actual_size);

    tc_ft_destroy(ft);
  }

  // Go through and make sure things are equal
  for (size_t i = 0; i < v.size(); ++i) {
    tc_flexible_type* ft = tc_flex_list_extract_element(fl, i, &error);

    TS_ASSERT(error == NULL);

    TS_ASSERT(tc_ft_is_double(ft) != 0);

    double val = tc_ft_double(ft, &error);
    TS_ASSERT(error == NULL);

    TS_ASSERT(v[i] == val);

    tc_ft_destroy(ft);
  }
}

