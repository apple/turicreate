#include "stdio.h"
#ifdef CMAKE_HAS_X

#  include <X11/Xlib.h>
#  include <X11/Xutil.h>

int main()
{
  printf("There is X on this computer\n");
  return 0;
}

#else

int main()
{
  printf("No X on this computer\n");
  return 0;
}

#endif
