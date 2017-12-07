
#ifdef TEST_LANG_DEFINES
#include "c_only.h"

#ifndef C_ONLY_DEFINE
#error Expected C_ONLY_DEFINE
#endif
#endif

int consumer_c()
{
  return 0;
}
