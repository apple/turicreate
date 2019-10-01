
#ifndef FOO_LIBRARY
#  error Expected FOO_LIBRARY
#endif

#ifndef BAR_LIBRARY
#  error Expected BAR_LIBRARY
#endif

#ifdef BANG_LIBRARY
#  error Unexpected BANG_LIBRARY
#endif

#ifdef ZOT_LIBRARY
#  error Unexpected ZOT_LIBRARY
#endif

#include "zot.h"

int main(void)
{
  return foo() + bar() + zot();
}
