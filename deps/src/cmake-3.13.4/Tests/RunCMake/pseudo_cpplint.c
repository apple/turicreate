#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
  int error = 0;
  int i;
  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--error") == 0) {
      error = 1;
    }
    if (argv[i][0] != '-') {
      if (error) {
        fprintf(stderr, "%s:0:  message  [category/category] [0]\n", argv[i]);
      }
      fprintf(stdout, "Done processing %s\nTotal errors found: %i\n", argv[i],
              error);
    }
  }
  return error;
}
