/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int testFail(int argc, char* argv[])
{
  char* env = getenv("DASHBOARD_TEST_FROM_CTEST");
  int oldCtest = 0;
  if (env) {
    if (strcmp(env, "1") == 0) {
      oldCtest = 1;
    }
    printf("DASHBOARD_TEST_FROM_CTEST = %s\n", env);
  }
  printf("%s: This test intentionally fails\n", argv[0]);
  if (oldCtest) {
    printf("The version of ctest is not able to handle intentionally failing "
           "tests, so pass.\n");
    return 0;
  }
  return argc;
}
