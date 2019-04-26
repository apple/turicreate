#include <stdio.h>

#if defined(_WIN32) && !defined(GREETING_STATIC)
__declspec(dllexport)
#endif
  void greeting(void)
{
  printf("Hello world!\n");
}
