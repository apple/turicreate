
#ifdef BAR_LIBRARY
#error Unexpected BAR_LIBRARY
#endif

#ifdef BANG_LIBRARY
#error Unexpected BANG_LIBRARY
#endif

#include "foo.h"

int foo()
{
  return 0;
}
