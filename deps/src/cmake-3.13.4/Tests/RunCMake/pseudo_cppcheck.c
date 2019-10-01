#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
  int i;
  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-bad") == 0)
      if (strcmp(argv[i], "-bad") == 0) {
        fprintf(stdout, "stdout from bad command line arg '-bad'\n");
        fprintf(stderr, "stderr from bad command line arg '-bad'\n");
        return 1;
      }
  }
  fprintf(stderr,
          "[/foo/bar.c:2]: (error) Array 'abc[10]' accessed at index 12,"
          " which is out of bounds.\n");
  fprintf(stderr,
          "[/foo/bar.c:2]: (warning) Member variable 'foo::bar' is "
          "not initialized in the constructor.\n");
  fprintf(stderr, "[/foo/bar.c:2]: (style) C-style pointer casting.\n");
  fprintf(stderr,
          "[/foo/bar.c:2]: (performance) Variable 'm_message' is "
          "assigned in constructor body. Consider performing "
          "initialization in initialization list.\n");
  fprintf(stderr,
          "[/foo/bar.c:2]: (portability) scanf without field width "
          "limits can crash with huge input data on some versions of "
          "libc\n");
  fprintf(stderr,
          "[/foo/bar.c:2]: (information) cannot find all the include "
          "files (use --check-config for details)\n");
  // we allow this to return 1 as we ignore it
  return 1;
}
