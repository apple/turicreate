#ifdef _WIN32
__declspec(dllimport)
#endif
  void foo(void);

#include <stdio.h>

void bar(void)
{
  foo();
  printf("bar()\n");
}
