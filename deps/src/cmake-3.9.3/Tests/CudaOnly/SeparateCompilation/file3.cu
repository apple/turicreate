

#include "file1.h"
#include "file2.h"

result_type __device__ file1_func(int x);
result_type_dynamic __device__ file2_func(int x);

static __global__ void file3_kernel(result_type& r, int x)
{
  // call static_func which is a method that is defined in the
  // static library that is always out of date
  r = file1_func(x);
  result_type_dynamic rd = file2_func(x);
}

result_type file3_launch_kernel(int x)
{
  result_type r;
  file3_kernel<<<1, 1>>>(r, x);
  return r;
}
