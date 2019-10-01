#include <cuda.h>

#include <iostream>

extern "C" int use_cuda(void)
{
  int nDevices = 0;
  cudaError_t err = cudaGetDeviceCount(&nDevices);
  if (err != cudaSuccess) {
    std::cerr << "Failed to retrieve the number of CUDA enabled devices"
              << std::endl;
    return 1;
  }
  std::cout << "Found " << nDevices << " CUDA enabled devices" << std::endl;
  return 0;
}
