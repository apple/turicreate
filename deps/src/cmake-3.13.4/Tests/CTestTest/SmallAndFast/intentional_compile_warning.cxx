#include <stdio.h>

int main(int argc, const char* argv[])
{
  unsigned int i =
    0; // "i<argc" should produce a "signed/unsigned comparison" warning
  for (; i < argc; ++i) {
    fprintf(stdout, "%s\n", argv[i]);
  }
  return 0;
}
