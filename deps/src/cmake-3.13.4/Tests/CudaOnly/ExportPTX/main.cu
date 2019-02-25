
#include <iostream>

/*
  Define GENERATED_HEADER macro to allow c++ files to include headers
  generated based on different configuration types.
*/

/* clang-format off */
#define GENERATED_HEADER(x) GENERATED_HEADER0(CONFIG_TYPE/x)
/* clang-format on */
#define GENERATED_HEADER0(x) GENERATED_HEADER1(x)
#define GENERATED_HEADER1(x) <x>

#include GENERATED_HEADER(path_to_objs.h)

#include "embedded_objs.h"

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  unsigned char* ka = kernelA;
  unsigned char* kb = kernelB;

  return (ka != NULL && kb != NULL) ? 0 : 1;
}
