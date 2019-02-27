
#include "test_compiler_detection.h"

#if !defined(TEST_COMPILER_C_FUNCTION_PROTOTYPES) ||                          \
  !TEST_COMPILER_C_FUNCTION_PROTOTYPES
#  error Expected TEST_COMPILER_C_FUNCTION_PROTOTYPES
#endif

#if !EXPECTED_COMPILER_C_FUNCTION_PROTOTYPES
#  error Expected EXPECTED_COMPILER_C_FUNCTION_PROTOTYPES
#endif

#if !defined(TEST_COMPILER_C_RESTRICT) || !TEST_COMPILER_C_RESTRICT
#  if EXPECTED_COMPILER_C_RESTRICT
#    error Expected TEST_COMPILER_C_RESTRICT
#  endif
#else
#  if !EXPECTED_COMPILER_C_RESTRICT
#    error Expect no TEST_COMPILER_C_RESTRICT
#  endif
#endif

#ifdef TEST_COMPILER_CXX_STATIC_ASSERT
#  error Expect no CXX features defined
#endif

int main()
{
  return 0;
}
