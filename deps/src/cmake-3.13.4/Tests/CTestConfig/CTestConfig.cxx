#include <stdio.h>

int main(int argc, const char* argv[])
{
  int i = 0;
  for (; i < argc; ++i) {
    fprintf(stdout, "%s\n", argv[i]);
  }

#ifdef CMAKE_BUILD_TYPE
  fprintf(stdout, "CMAKE_BUILD_TYPE is %s\n", CMAKE_BUILD_TYPE);
#endif

#ifdef CMAKE_INTDIR
  fprintf(stdout, "CMAKE_INTDIR is %s\n", CMAKE_INTDIR);
#endif

  return 0;
}
