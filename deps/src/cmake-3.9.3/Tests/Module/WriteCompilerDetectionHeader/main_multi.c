
#include "multi_file_compiler_detection.h"

#if !defined(MULTI_COMPILER_C_FUNCTION_PROTOTYPES) ||                         \
  !MULTI_COMPILER_C_FUNCTION_PROTOTYPES
#error Expected MULTI_COMPILER_C_FUNCTION_PROTOTYPES
#endif

#if !EXPECTED_COMPILER_C_FUNCTION_PROTOTYPES
#error Expected EXPECTED_COMPILER_C_FUNCTION_PROTOTYPES
#endif

#if !defined(MULTI_COMPILER_C_RESTRICT) || !MULTI_COMPILER_C_RESTRICT
#if EXPECTED_COMPILER_C_RESTRICT
#error Expected MULTI_COMPILER_C_RESTRICT
#endif
#else
#if !EXPECTED_COMPILER_C_RESTRICT
#error Expect no MULTI_COMPILER_C_RESTRICT
#endif
#endif

#ifdef MULTI_COMPILER_CXX_STATIC_ASSERT
#error Expect no CXX features defined
#endif

int main()
{
  return 0;
}
