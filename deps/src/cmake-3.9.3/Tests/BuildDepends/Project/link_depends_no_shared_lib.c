#include "link_depends_no_shared_lib.h"
#ifdef _WIN32
__declspec(dllexport)
#endif
  int link_depends_no_shared_lib(void)
{
  return link_depends_no_shared_lib_value;
}
