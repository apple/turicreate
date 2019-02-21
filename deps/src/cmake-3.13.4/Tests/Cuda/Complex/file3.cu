
#include <iostream>

#include "file1.h"
#include "file2.h"

result_type __device__ file1_func(int x);
result_type_dynamic __device__ file2_func(int x);

static __global__ void file3_kernel(result_type* r, int x)
{
  *r = file1_func(x);
  result_type_dynamic rd = file2_func(x);
}

int file3_launch_kernel(int x)
{
  result_type* r;
  cudaError_t err = cudaMallocManaged(&r, sizeof(result_type));
  if (err != cudaSuccess) {
    std::cerr << "file3_launch_kernel: cudaMallocManaged failed: "
              << cudaGetErrorString(err) << std::endl;
    return x;
  }

  file3_kernel<<<1, 1>>>(r, x);
  err = cudaGetLastError();
  if (err != cudaSuccess) {
    std::cerr << "file3_kernel [SYNC] failed: " << cudaGetErrorString(err)
              << std::endl;
    return x;
  }
  err = cudaDeviceSynchronize();
  if (err != cudaSuccess) {
    std::cerr << "file3_kernel [ASYNC] failed: "
              << cudaGetErrorString(cudaGetLastError()) << std::endl;
    return x;
  }
  int result = r->sum;
  err = cudaFree(r);
  if (err != cudaSuccess) {
    std::cerr << "file3_launch_kernel: cudaFree failed: "
              << cudaGetErrorString(err) << std::endl;
    return x;
  }

  return result;
}
