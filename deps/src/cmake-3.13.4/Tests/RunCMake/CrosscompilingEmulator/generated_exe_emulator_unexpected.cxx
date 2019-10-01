#include <stdio.h>

int main(int argc, const char* argv[])
{
  for (int i = 1; i < argc; ++i) {
    fprintf(stderr, "unexpected argument: '%s'\n", argv[i]);
  }
  return argc == 1 ? 0 : 1;
}
