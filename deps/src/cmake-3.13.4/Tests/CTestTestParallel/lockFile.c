#include <stdio.h>

/*if run serially, works fine.
  If run in parallel, someone will attempt to delete
  a locked file, which will fail */
int main(void)
{
  FILE* file;
  int i;
  const char* fname = "lockedFile.txt";
  file = fopen(fname, "w");

  for (i = 0; i < 10000; i++) {
    fprintf(file, "%s", "x");
    fflush(file);
  }
  fclose(file);
  return remove(fname);
}
