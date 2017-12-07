
#include "common.h"
#include "privateinclude.h"
#include "publicinclude.h"

#ifndef PRIVATEINCLUDE_DEFINE
#error Expected PRIVATEINCLUDE_DEFINE
#endif

#ifndef PUBLICINCLUDE_DEFINE
#error Expected PUBLICINCLUDE_DEFINE
#endif

#ifdef INTERFACEINCLUDE_DEFINE
#error Unexpected INTERFACEINCLUDE_DEFINE
#endif

#ifndef CURE_DEFINE
#error Expected CURE_DEFINE
#endif

int main()
{
  return 0;
}
