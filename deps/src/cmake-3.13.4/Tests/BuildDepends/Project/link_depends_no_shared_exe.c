#include "link_depends_no_shared_exe.h"
#ifdef _WIN32
__declspec(dllimport)
#endif
  int link_depends_no_shared_lib(void);
int main()
{
  return link_depends_no_shared_lib() + link_depends_no_shared_exe_value;
}
