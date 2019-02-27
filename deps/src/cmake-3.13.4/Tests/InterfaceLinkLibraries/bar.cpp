
#ifdef FOO_LIBRARY
#  error Unexpected FOO_LIBRARY
#endif

#ifdef BAR_USE_BANG
#  ifndef BANG_LIBRARY
#    error Expected BANG_LIBRARY
#  endif
#  include "bang.h"
#else
#  ifdef BANG_LIBRARY
#    error Unexpected BANG_LIBRARY
#  endif
#endif

#include "bar.h"

int bar()
{
#ifdef BAR_USE_BANG
  return bang();
#else
  return 0;
#endif
}
