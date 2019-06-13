/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#include <capi/TuriCreate.h>
#include <vector>
#include "capi_utils.hpp"

struct capi_test_parameters {

 void test_parameters_flexible_type() {
  
   tc_error* error = NULL;


   tc_parameters* p = tc_parameters_create_empty(&error);
   CAPI_CHECK_ERROR(error);

   tc_flexible_type* ft = tc_ft_create_from_double(9.0, &error);
   CAPI_CHECK_ERROR(error);

   tc_parameters_add_flexible_type(p, "arg1", ft, &error); 
   CAPI_CHECK_ERROR(error);

   tc_release(ft);

   int is_ft = tc_parameters_is_flexible_type(p, "arg1", &error); 
   CAPI_CHECK_ERROR(error);
   TS_ASSERT(is_ft == 1); 


   ft = tc_parameters_retrieve_flexible_type(p, "arg1", &error); 
   CAPI_CHECK_ERROR(error);

   int is_dbl = tc_ft_is_double(ft); 
   TS_ASSERT(is_dbl);

   double x = tc_ft_double(ft, &error); 
   CAPI_CHECK_ERROR(error);
   TS_ASSERT_EQUALS(x, 9.0);
   tc_release(ft);

   is_dbl = tc_parameters_is_double(p, "arg1", &error);
   CAPI_CHECK_ERROR(error);
   TS_ASSERT(is_dbl);

   x = tc_parameters_retrieve_double(p, "arg1", &error);
   CAPI_CHECK_ERROR(error);
   TS_ASSERT_EQUALS(x, 9.0);

   tc_release(p);
 }
 
 void test_parameters_sarray() {
  
   tc_error* error = NULL;

   tc_parameters* p = tc_parameters_create_empty(&error);
   CAPI_CHECK_ERROR(error);

   tc_sarray* sa = make_sarray_double({1.0, 2.0, 4.0});

   tc_parameters_add_sarray(p, "arg1", sa, &error); 
   CAPI_CHECK_ERROR(error);

   tc_release(sa);

   int is_sa = tc_parameters_is_sarray(p, "arg1", &error); 
   CAPI_CHECK_ERROR(error);
   TS_ASSERT(is_sa == 1); 

   sa = tc_parameters_retrieve_sarray(p, "arg1", &error); 

   size_t size = tc_sarray_size(sa); 

   TS_ASSERT_EQUALS(size, 3);

   tc_release(sa);

   tc_release(p);
  
 }
 
 void test_parameters_sframe() {
  
   tc_error* error = NULL;

   tc_parameters* p = tc_parameters_create_empty(&error);
   CAPI_CHECK_ERROR(error);

   tc_sframe* sf = make_sframe_double(
       { {"col1", {1.0, 2.0, 4.0}}, 
         {"col2", {2.0, 3.0, 0.0}}
       });

   tc_parameters_add_sframe(p, "arg1", sf, &error); 
   CAPI_CHECK_ERROR(error);

   tc_release(sf);

   int is_sf = tc_parameters_is_sframe(p, "arg1", &error); 
   CAPI_CHECK_ERROR(error);
   TS_ASSERT(is_sf == 1); 

   sf = tc_parameters_retrieve_sframe(p, "arg1", &error); 

   size_t size = tc_sframe_num_rows(sf, &error); 

   TS_ASSERT_EQUALS(size, 3); 

   size_t cols = tc_sframe_num_columns(sf, &error);

   TS_ASSERT_EQUALS(cols, 2); 

   tc_release(sf);
   tc_release(p);
 }


};

BOOST_FIXTURE_TEST_SUITE(_capi_test_parameters, capi_test_parameters)
BOOST_AUTO_TEST_CASE(test_parameters_flexible_type) {
  capi_test_parameters::test_parameters_flexible_type();
}
BOOST_AUTO_TEST_CASE(test_parameters_sarray) {
  capi_test_parameters::test_parameters_sarray();
}
BOOST_AUTO_TEST_CASE(test_parameters_sframe) {
  capi_test_parameters::test_parameters_sframe();
}
BOOST_AUTO_TEST_SUITE_END()
