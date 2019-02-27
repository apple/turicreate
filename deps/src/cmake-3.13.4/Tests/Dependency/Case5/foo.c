#include <stdio.h>

#ifdef _WIN32
__declspec(dllexport)
#endif
  void foo(void)
{
  printf("foo()\n");
}
