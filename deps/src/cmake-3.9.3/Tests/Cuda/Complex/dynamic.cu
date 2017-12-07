
#include <cuda.h>
#include <iostream>
#include <string>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

int dynamic_base_func(int);

EXPORT int __host__ cuda_dynamic_host_func(int x)
{
  return dynamic_base_func(x);
}

static __global__ void DetermineIfValidCudaDevice()
{
}

EXPORT int choose_cuda_device()
{
  int nDevices = 0;
  cudaError_t err = cudaGetDeviceCount(&nDevices);
  if (err != cudaSuccess) {
    std::cerr << "Failed to retrieve the number of CUDA enabled devices"
              << std::endl;
    return 1;
  }
  for (int i = 0; i < nDevices; ++i) {
    cudaDeviceProp prop;
    cudaError_t err = cudaGetDeviceProperties(&prop, i);
    if (err != cudaSuccess) {
      std::cerr << "Could not retrieve properties from CUDA device " << i
                << std::endl;
      return 1;
    }
    if (prop.major >= 3) {
      err = cudaSetDevice(i);
      if (err != cudaSuccess) {
        std::cout << "Could not select CUDA device " << i << std::endl;
      } else {
        return 0;
      }
    }
  }

  std::cout << "Could not find a CUDA enabled card supporting compute >=3.0"
            << std::endl;

  return 1;
}

EXPORT void cuda_dynamic_lib_func()
{
  DetermineIfValidCudaDevice<<<1, 1>>>();
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    std::cerr << "DetermineIfValidCudaDevice [SYNC] failed: "
              << cudaGetErrorString(err) << std::endl;
  }
  err = cudaDeviceSynchronize();
  if (err != cudaSuccess) {
    std::cerr << "DetermineIfValidCudaDevice [ASYNC] failed: "
              << cudaGetErrorString(cudaGetLastError()) << std::endl;
  }
}
