#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
#ifndef SRC_GENEX_WORKS
#error SRC_GENEX_WORKS not defined
#endif
  printf("%s\n", argv[1]);
  return EXIT_SUCCESS;
}
