#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
  char* var = getenv("CMAKE_ENVIRONMENT_TEST_VAR");
  if (!var) {
    var = "(null)";
  }

  fprintf(stdout, "Environment:\n");
  fprintf(stdout, "  CMAKE_ENVIRONMENT_TEST_VAR='%s'\n", var);

  return 0;
}
