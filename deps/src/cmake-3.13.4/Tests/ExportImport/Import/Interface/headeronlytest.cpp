
#include "headeronly.h"

#ifndef HEADERONLY_DEFINE
#  error Expected HEADERONLY_DEFINE
#endif

#ifdef SHAREDLIB_DEFINE
#  error Unexpected SHAREDLIB_DEFINE
#endif

int main(int, char**)
{
  HeaderOnly ho;
  return ho.foo();
}
