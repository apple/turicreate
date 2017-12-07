
#include "consumer.h"
#include "common.h"
#include "interfaceinclude.h"
#include "publicinclude.h"
#include "relative_dir.h"
#ifdef TEST_LANG_DEFINES
#include "cxx_only.h"
#endif

#ifdef PRIVATEINCLUDE_DEFINE
#error Unexpected PRIVATEINCLUDE_DEFINE
#endif

#ifndef PUBLICINCLUDE_DEFINE
#error Expected PUBLICINCLUDE_DEFINE
#endif

#ifndef INTERFACEINCLUDE_DEFINE
#error Expected INTERFACEINCLUDE_DEFINE
#endif

#ifndef CURE_DEFINE
#error Expected CURE_DEFINE
#endif

#ifndef RELATIVE_DIR_DEFINE
#error Expected RELATIVE_DIR_DEFINE
#endif

#ifndef CONSUMER_DEFINE
#error Expected CONSUMER_DEFINE
#endif

#ifdef TEST_LANG_DEFINES
#ifndef CXX_ONLY_DEFINE
#error Expected CXX_ONLY_DEFINE
#endif
#endif

int main()
{
  return 0;
}
