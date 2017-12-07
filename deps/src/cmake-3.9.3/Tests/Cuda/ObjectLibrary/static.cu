
#include <cuda.h>
#include <cuda_runtime.h>
#include <iostream>

int __host__ file1_sq_func(int x)
{
  cudaError_t err;
  int nDevices = 0;
  err = cudaGetDeviceCount(&nDevices);
  if (err != cudaSuccess) {
    std::cerr << "nDevices: " << nDevices << std::endl;
    std::cerr << "err: " << err << std::endl;
    return 1;
  }
  std::cout << "this library uses cuda code" << std::endl;
  std::cout << "you have " << nDevices << " devices that support cuda"
            << std::endl;

  return x * x;
}
