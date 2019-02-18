#include <stdio.h>

int main(int argc, char* argv[])
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return 1;
  }
  FILE* fp = fopen(argv[1], "w");
#ifdef GENERATOR_EXTERN
  fprintf(fp, "int generated() { return 3; }\n");
#else
  fprintf(fp, "extern int gen_redirect(void);\n");
  fprintf(fp, "int generated() { return gen_redirect(); }\n");
#endif
  fclose(fp);
  return 0;
}
