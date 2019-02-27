#include <stdio.h>

#ifdef __CLASSIC_C__
int main(argc, argv) int argc;
char** argv;
#else
int main(int argc, const char* argv[])
#endif
{
  FILE* fout = fopen(argv[argc - 1], "w");
  printf("Wrap creating \"%s\"\n", argv[argc - 1]);
  if (fout) {
    fprintf(fout, "int foo() { return 0; }\n");
    fclose(fout);
  } else {
    return -1;
  }
  return 0;
}
