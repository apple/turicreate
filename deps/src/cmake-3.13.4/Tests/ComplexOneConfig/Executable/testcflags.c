#include <string.h>

int TestTargetCompileFlags(char* m)
{
#ifndef COMPLEX_TARGET_FLAG
  strcpy(m, "CMAKE SET_TARGET_PROPERTIES COMPILE_FLAGS did not work");
  return 0;
#endif
  strcpy(m, "CMAKE SET_TARGET_PROPERTIES COMPILE_FLAGS worked");
  return 1;
}

int TestCFlags(char* m)
{
/* TEST_CXX_FLAGS should not be defined in a c file */
#ifdef TEST_CXX_FLAGS
  strcpy(m, "CMake CMAKE_CXX_FLAGS (TEST_CXX_FLAGS) found in c file.");
  return 0;
#endif
/* TEST_C_FLAGS should be defined in a c file */
#ifndef TEST_C_FLAGS
  strcpy(m, "CMake CMAKE_C_FLAGS (TEST_C_FLAGS) not found in c file.");
  return 0;
#endif
  return 1;
}
