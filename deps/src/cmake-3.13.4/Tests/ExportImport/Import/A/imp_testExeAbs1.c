#include "testLibAbs1.h"
#include "testLibAbs1a.h"
#include "testLibAbs1b.h"
#ifndef testLibAbs1a
#  error "testLibAbs1a not defined"
#endif
#ifndef testLibAbs1b
#  error "testLibAbs1b not defined"
#endif
int main()
{
  return 0 + testLibAbs1();
}
