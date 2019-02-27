
#include "file2.h"

result_type __device__ file1_func(int x);

result_type_dynamic __device__ file2_func(int x)
{
  const result_type r = file1_func(x);
  const result_type_dynamic rd{ r.input, r.sum, true };
  return rd;
}

static __global__ void file2_kernel(result_type_dynamic& r, int x)
{
  // call static_func which is a method that is defined in the
  // static library that is always out of date
  r = file2_func(x);
}

int file2_launch_kernel(int x)
{
  result_type_dynamic r;
  file2_kernel<<<1, 1>>>(r, x);
  return r.sum;
}
