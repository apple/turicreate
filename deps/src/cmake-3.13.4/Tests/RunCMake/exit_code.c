#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Usage:
//
//  /path/to/program arg1 [arg2 [...]]
//
// Return EXIT_SUCCESS if 'zero_exit'
// string was found in <arg1>.
// Return EXIT_FAILURE if 'non_zero_exit'
// string was found in <arg1>.

int main(int argc, const char* argv[])
{
  const char* substring_failure = "non_zero_exit";
  const char* substring_success = "zero_exit";
  const char* str = argv[1];
  if (argc < 2) {
    return EXIT_FAILURE;
  }
  if (strcmp(str, substring_success) == 0) {
    return EXIT_SUCCESS;
  } else if (strcmp(str, substring_failure) == 0) {
    return EXIT_FAILURE;
  }
  fprintf(stderr, "Failed to find string '%s' in '%s'\n", substring_success,
          str);
  return EXIT_FAILURE;
}
