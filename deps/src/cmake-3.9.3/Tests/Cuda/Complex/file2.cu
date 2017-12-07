
#include "file2.h"

result_type __device__ file1_func(int x);

result_type_dynamic __device__ file2_func(int x)
{
  if (x != 42) {
    const result_type r = file1_func(x);
    const result_type_dynamic rd{ r.input, r.sum, true };
    return rd;
  } else {
    const result_type_dynamic rd{ x, x * x * x, false };
    return rd;
  }
}
