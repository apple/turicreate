
#include "foo.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
  int bar();
