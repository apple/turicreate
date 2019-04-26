#ifdef _MSC_VER
#  include "windows.h"
#else
#  define WINAPI
#endif

int WINAPI foo()
{
  return 10;
}

int bar()
{
  return 5;
}
