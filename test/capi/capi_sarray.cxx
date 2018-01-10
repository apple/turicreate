#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <capi/TuriCore.h>
#include <vector>
#include "capi_utils.hpp"

class capi_test_sarray {
 public:

 void test_sarray_double() {

    std::vector<double> v = {1, 2, 4.5, 9, 10000000, -12433};

    tc_error* error = NULL; 
    
    tc_flex_list* fl = make_flex_list_double(v); 

    tc_sarray* sa = tc_sarray_create_from_list(fl, &error);

    TS_ASSERT(error == NULL); 

    { 
      // Make sure it gets out what we want it to.
      for(size_t i = 0; i < v.size(); ++i) { 
        tc_flexible_type* ft = tc_sarray_extract_element(sa, i, &error); 
        TS_ASSERT(error == NULL); 

        TS_ASSERT(tc_ft_is_double(ft) != 0);

        double val = tc_ft_double(ft, &error); 
        TS_ASSERT(error == NULL); 

        TS_ASSERT(v[i] == val); 
      
        tc_ft_destroy(ft);
      }
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_capi_test_sarray, capi_test_sarray)
BOOST_AUTO_TEST_CASE(test_sarray_double) {
  capi_test_sarray::test_sarray_double();
}
BOOST_AUTO_TEST_SUITE_END()
