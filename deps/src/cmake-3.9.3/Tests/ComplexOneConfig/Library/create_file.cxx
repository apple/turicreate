#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
  if (argc < 2) {
    fprintf(stderr, "Missing name of file to create.\n");
    return EXIT_FAILURE;
  }

  FILE* stream = fopen(argv[1], "w");
  if (stream == NULL) {
    fprintf(stderr, "Unable to open %s for writing!\n", argv[1]);
    return EXIT_FAILURE;
  }

  if (fclose(stream)) {
    fprintf(stderr, "Unable to close %s!\n", argv[1]);
    return EXIT_FAILURE;
  }

  fprintf(stdout, ">> Creating %s!\n", argv[1]);

  return EXIT_SUCCESS;
}
