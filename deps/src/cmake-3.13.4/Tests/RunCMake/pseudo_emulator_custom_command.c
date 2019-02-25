#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Usage:
//
//  /path/to/program arg1 [arg2 [...]]
//
// Return EXIT_SUCCESS if 'generated_exe_emulator_expected'
// string was found in <arg1>.
// Return EXIT_FAILURE if 'generated_exe_emulator_unexpected'
// string was found in <arg1>.

int main(int argc, const char* argv[])
{
  const char* substring_failure = "generated_exe_emulator_unexpected";
  // Require a slash to make sure it is a path and not a target name.
  const char* substring_success = "/generated_exe_emulator_expected";
  const char* str = argv[1];
  if (argc < 2) {
    return EXIT_FAILURE;
  }
  if (strstr(str, substring_success) != 0) {
    return EXIT_SUCCESS;
  }
  if (strstr(str, substring_failure) != 0) {
    return EXIT_FAILURE;
  }
  fprintf(stderr, "Failed to find string '%s' in '%s'\n", substring_success,
          str);
  return EXIT_FAILURE;
}
