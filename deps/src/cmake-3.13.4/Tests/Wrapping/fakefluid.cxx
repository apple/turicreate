#include <stdio.h>
#include <string.h>
int main(int ac, char** av)
{
  for (int i = 0; i < ac; ++i) {
    if (strcmp(av[i], "-o") == 0 || strcmp(av[i], "-h") == 0) {
      fprintf(stdout, "fakefluid is creating file \"%s\"\n", av[i + 1]);
      FILE* file = fopen(av[i + 1], "w");
      fprintf(file,
              "// Solaris needs non-empty content so ensure\n"
              "// we have at least one symbol\n"
              "int Solaris_requires_a_symbol_here = 0;\n");
      fclose(file);
    }
  }
  return 0;
}
