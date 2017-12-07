#include <iostream>

#include "file1.h"
#include "file2.h"

#ifdef _WIN32
#define IMPORT __declspec(dllimport)
#else
#define IMPORT
#endif

IMPORT int choose_cuda_device();
IMPORT int call_cuda_seperable_code(int x);
IMPORT int mixed_launch_kernel(int x);

int main(int argc, char** argv)
{
  int ret = choose_cuda_device();
  if (ret) {
    return 0;
  }

  int r1 = call_cuda_seperable_code(42);
  int r2 = mixed_launch_kernel(42);
  return (r1 == 42 || r2 == 42) ? 1 : 0;
}
