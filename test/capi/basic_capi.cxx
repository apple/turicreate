#include <capi/TuriCore.h>

class test_flexible_type : public CxxTest::TestSuite {
 public:

  void test_flex_double() {
    tc_error* error = NULL; 

    tc_flexible_type* ft = tc_ft_create_from_double(1, &error);

    TS_ASSERT(error == NULL); 
    TS_ASSERT(ft != NULL); 

  }
};

