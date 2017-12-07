
#include <iostream>

#include "file1.h"
#include "file2.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#else
#define EXPORT
#define IMPORT
#endif

result_type __device__ file1_func(int x);
result_type_dynamic __device__ file2_func(int x);

IMPORT void __host__ cuda_dynamic_lib_func();

static __global__ void mixed_kernel(result_type* r, int x)
{
  *r = file1_func(x);
  result_type_dynamic rd = file2_func(x);
}

EXPORT int mixed_launch_kernel(int x)
{
  cuda_dynamic_lib_func();

  result_type* r;
  cudaError_t err = cudaMallocManaged(&r, sizeof(result_type));
  if (err != cudaSuccess) {
    std::cerr << "mixed_launch_kernel: cudaMallocManaged failed: "
              << cudaGetErrorString(err) << std::endl;
    return x;
  }

  mixed_kernel<<<1, 1>>>(r, x);
  err = cudaGetLastError();
  if (err != cudaSuccess) {
    std::cerr << "mixed_kernel [SYNC] failed: " << cudaGetErrorString(err)
              << std::endl;
    return x;
  }
  err = cudaDeviceSynchronize();
  if (err != cudaSuccess) {
    std::cerr << "mixed_kernel [ASYNC] failed: "
              << cudaGetErrorString(cudaGetLastError()) << std::endl;
    return x;
  }

  int result = r->sum;
  err = cudaFree(r);
  if (err != cudaSuccess) {
    std::cerr << "mixed_launch_kernel: cudaFree failed: "
              << cudaGetErrorString(err) << std::endl;
    return x;
  }

  return result;
}
