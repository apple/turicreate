
#include "multi_file_compiler_detection.h"

#define PREFIX MULTI
#include "compile_tests.h"

#ifdef MULTI_COMPILER_C_STATIC_ASSERT
#error Expect no C features defined
#endif

MULTI_STATIC_ASSERT(true);
MULTI_STATIC_ASSERT_MSG(true, "msg");

int main()
{
  return 0;
}
