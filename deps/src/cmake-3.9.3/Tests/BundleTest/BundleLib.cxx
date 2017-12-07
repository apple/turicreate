#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>

int fileExists(char* filename)
{
#ifndef R_OK
#define R_OK 04
#endif
  if (access(filename, R_OK) != 0) {
    printf("Cannot find file: %s\n", filename);
    return 0;
  }
  return 1;
}

int findBundleFile(char* exec, const char* file)
{
  int res;
  char* nexec = strdup(exec);
  char* fpath = (char*)malloc(strlen(exec) + 100);
  int cc;
  int cnt = 0;
  printf("Process executable name: %s\n", exec);

  // Remove the executable name and directory name
  for (cc = strlen(nexec) - 1; cc > 0; cc--) {
    if (nexec[cc] == '/') {
      nexec[cc] = 0;
      if (cnt == 1) {
        break;
      }
      cnt++;
    }
  }
  printf("Process executable path: %s\n", nexec);
  sprintf(fpath, "%s/%s", nexec, file);
  printf("Check for file: %s\n", fpath);
  res = fileExists(fpath);
  free(nexec);
  free(fpath);
  return res;
}

int foo(char* exec)
{
  // Call a CoreFoundation function...
  //
  CFBundleRef br = CFBundleGetMainBundle();
  (void)br;

  int res1 = findBundleFile(exec, "Resources/randomResourceFile.plist");
  int res2 = findBundleFile(exec, "MacOS/SomeRandomFile.txt");
  int res3 = findBundleFile(exec, "MacOS/README.rst");
  if (!res1 || !res2 || !res3) {
    return 1;
  }

  return 0;
}
