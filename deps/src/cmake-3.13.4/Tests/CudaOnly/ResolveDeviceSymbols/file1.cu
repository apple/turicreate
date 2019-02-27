
#include "file1.h"

result_type __device__ file1_func(int x)
{
  result_type r;
  r.input = x;
  r.sum = x * x;
  return r;
}
