#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

// return true if the file exists
int FileExists(const char* filename)
{
#ifdef _MSC_VER
#define access _access
#endif
#ifndef F_OK
#define F_OK 0
#endif
  if (access(filename, F_OK) != 0) {
    return false;
  } else {
    return true;
  }
}

int main(int ac, char** av)
{
  if (ac <= 1) {
    printf("Usage: %s <file>\n", av[0]);
    return 1;
  }
  if (!FileExists(av[1])) {
    printf("Missing file %s\n", av[1]);
    return 1;
  }
  if (FileExists(av[2])) {
    printf("File %s should be in subdirectory\n", av[2]);
    return 1;
  }
  printf("%s is not there! Good.", av[2]);
  printf("%s is there! Good.", av[1]);
  return 0;
}
