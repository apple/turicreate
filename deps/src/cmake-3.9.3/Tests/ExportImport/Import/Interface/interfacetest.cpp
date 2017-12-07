
#include "sharedlib.h"

#ifndef SHAREDLIB_DEFINE
#error Expected SHAREDLIB_DEFINE
#endif

#ifdef HEADERONLY_DEFINE
#error Unexpected HEADERONLY_DEFINE
#endif

#ifndef DEFINE_IFACE_DEFINE
#error Expected DEFINE_IFACE_DEFINE
#endif

int main(int, char**)
{
  SharedLibObject slo;
  return slo.foo();
}
