#include <stdio.h>

/* Disable deprecation warning for fopen */
#pragma warning(disable : 4996)

/*if run serially, works fine.
  If run in parallel, someone will attempt to delete
  a locked file, which will fail */
int main(int argc, char** argv)
{
  FILE* file;
  int i;
  const char* fname;
  if (argc >= 2) {
    fname = argv[1];
  } else {
    fname = "lockedFile.txt";
  }
  file = fopen(fname, "w");

  for (i = 0; i < 10000; i++) {
    fprintf(file, "%s", "x");
    fflush(file);
  }
  fclose(file);
  return remove(fname);
}
