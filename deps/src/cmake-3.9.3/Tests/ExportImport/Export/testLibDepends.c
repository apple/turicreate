
#include "testLibIncludeRequired1.h"
#include "testLibIncludeRequired2.h"
#include "testLibIncludeRequired4.h"

#ifndef testLibRequired_IFACE_DEFINE
#error Expected testLibRequired_IFACE_DEFINE
#endif

#ifndef BuildOnly_DEFINE
#error Expected BuildOnly_DEFINE
#endif

#ifdef InstallOnly_DEFINE
#error Unexpected InstallOnly_DEFINE
#endif

extern int testLibRequired(void);
extern int testStaticLibRequiredPrivate(void);

int testLibDepends(void)
{
  return testLibRequired() + testStaticLibRequiredPrivate();
}
