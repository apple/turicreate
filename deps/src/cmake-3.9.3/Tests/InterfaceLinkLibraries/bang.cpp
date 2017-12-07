
#ifdef FOO_LIBRARY
#error Unexpected FOO_LIBRARY
#endif

#ifdef BAR_LIBRARY
#error Unexpected BAR_LIBRARY
#endif

#include "bang.h"

int bang()
{
  return 0;
}
