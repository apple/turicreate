#include <stdio.h>
#if defined(_WIN32) && defined(foo_EXPORTS)
#  define CM_TEST_LIB_EXPORT __declspec(dllexport)
#else
#  define CM_TEST_LIB_EXPORT
#endif
CM_TEST_LIB_EXPORT void foo()
{
  printf("foo\n");
}
