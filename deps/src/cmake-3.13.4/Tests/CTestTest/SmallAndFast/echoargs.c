#include <stdio.h>

int main(int argc, const char* argv[])
{
  int i = 0;
  for (; i < argc; ++i) {
    fprintf(stdout, "%s\n", argv[i]);
  }
  return 0;
}
