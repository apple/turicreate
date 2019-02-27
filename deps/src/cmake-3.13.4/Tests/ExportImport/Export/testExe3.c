#include <stdio.h>

int main(int argc, const char* argv[])
{
  if (argc < 2) {
    fprintf(stderr, "Must specify output file.\n");
    return 1;
  }
  {
    FILE* f = fopen(argv[1], "w");
    if (f) {
      fprintf(f, "int generated_by_testExe3() { return 0; }\n");
      fclose(f);
    } else {
      fprintf(stderr, "Error writing to %s\n", argv[1]);
      return 1;
    }
  }
  return 0;
}
