#include "libcxx1.h"

#ifdef TEST_FLAG_1
#  ifdef TEST_FLAG_2
float LibCxx1Class::Method()
{
  return 2.0;
}
#  endif
#endif
