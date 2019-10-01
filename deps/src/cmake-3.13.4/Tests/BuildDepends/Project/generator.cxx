#include <stdio.h>

extern const char* foo();

int main(int argc, const char* argv[])
{
  if (argc < 3) {
    fprintf(stderr, "Must specify output file and symbol prefix!");
    return 1;
  }
  if (FILE* fout = fopen(argv[1], "w")) {
    fprintf(fout, "static const char* %s_string = \"%s\";\n", argv[2], foo());
    fclose(fout);
  } else {
    fprintf(stderr, "Could not open output file \"%s\"", argv[1]);
    return 1;
  }
  return 0;
}
