/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(Configure.hxx)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#  include "Configure.hxx.in"
#endif

static bool testFallthrough(int n)
{
  int r = 0;
  switch (n) {
    case 1:
      ++r;
      KWSYS_FALLTHROUGH;
    default:
      ++r;
  }
  return r == 2;
}

int testConfigure(int, char* [])
{
  bool res = true;
  res = testFallthrough(1) && res;
  return res ? 0 : 1;
}
