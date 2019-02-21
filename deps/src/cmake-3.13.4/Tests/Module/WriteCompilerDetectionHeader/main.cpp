
#include "test_compiler_detection.h"

#define PREFIX TEST
#include "compile_tests.h"

#ifdef TEST_COMPILER_C_STATIC_ASSERT
#  error Expect no C features defined
#endif

TEST_STATIC_ASSERT(true);
TEST_STATIC_ASSERT_MSG(true, "msg");

int main()
{
  return 0;
}
