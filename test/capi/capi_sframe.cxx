#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <capi/TuriCore.h>
#include <vector>
#include "capi_utils.hpp"

struct capi_test_sframe {
 public:

 void test_sframe_allocation() {
    tc_error* error = NULL; 

    tc_sframe* sf = tc_sframe_create_empty(&error); 
      
    TS_ASSERT(error == NULL); 
    
    tc_sframe_destroy(sf);
  }

void test_sframe_double() {

std::vector<std::pair<std::string, std::vector<double> > > data 
  = { {"col1", {1.0, 2., 5., 0.5} },
      {"col2", {2.0, 2., 3., 0.5} },
      {"a",    {5.0, 2., 1., 0.5} },
      {"b",    {7.0, 2., 3., 1.5} } };

    tc_error* error = NULL; 

    tc_sframe* sf = tc_sframe_create_empty(&error); 
      
    TS_ASSERT(error == NULL); 

    for(auto p : data) { 

      tc_sarray* sa = make_sarray_double(p.second); 

      tc_sframe_add_column(sf, p.first.c_str(), sa, &error); 

      TS_ASSERT(error == NULL); 

      tc_sarray_destroy(sa); 
    }

    // Check everything 
    for(auto p : data) { 
      // Make sure it gets out what we want it to.
      tc_sarray* sa = tc_sframe_extract_column_by_name(sf, p.first.c_str(), &error); 

      TS_ASSERT(error == NULL); 

      tc_sarray* ref_sa = make_sarray_double(p.second);

      int is_equal = tc_sarray_equals(sa, ref_sa, &error);     
      TS_ASSERT(is_equal);

      TS_ASSERT(error == NULL); 

      tc_sarray_destroy(sa); 
    }
    
    tc_sframe_destroy(sf);
  }
};

BOOST_FIXTURE_TEST_SUITE(_capi_test_sframe, capi_test_sframe)
BOOST_AUTO_TEST_CASE(test_sframe_allocation) {
 capi_test_sframe::test_sframe_allocation();
}
BOOST_AUTO_TEST_CASE(test_sframe_double) {
  capi_test_sframe::test_sframe_double();
}
BOOST_AUTO_TEST_SUITE_END()
