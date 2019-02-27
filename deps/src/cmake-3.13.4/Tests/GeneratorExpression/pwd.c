#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#  include <direct.h>
#  define getcurdir _getcwd
#else
#  include <unistd.h>
#  define getcurdir getcwd
#endif

int main(int argc, char* argv[])
{
#define BUFSZ 20000
  char buf[BUFSZ + 1];
#ifdef _WIN32
  char* pos;
#endif
  getcurdir(buf, BUFSZ);
#ifdef _WIN32
  pos = buf;
  while (*pos) {
    if (*pos == '\\') {
      *pos = '/';
    }
    ++pos;
  }
#endif
  printf("%s\n", buf);
  return EXIT_SUCCESS;
}
