
#include <iostream>

#include "file1.h"
#include "file2.h"

int file4_launch_kernel(int x);
int file5_launch_kernel(int x);

int choose_cuda_device()
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

int main(int argc, char** argv)
{
  int ret = choose_cuda_device();
  if (ret) {
    return 0;
  }

  cudaError_t err;
  file4_launch_kernel(42);
  err = cudaGetLastError();
  if (err != cudaSuccess) {
    std::cerr << "file4_launch_kernel: kernel launch failed: "
              << cudaGetErrorString(err) << std::endl;
    return 1;
  }

  file5_launch_kernel(42);
  err = cudaGetLastError();
  if (err != cudaSuccess) {
    std::cerr << "file5_launch_kernel: kernel launch failed: "
              << cudaGetErrorString(err) << std::endl;
    return 1;
  }

  return 0;
}
