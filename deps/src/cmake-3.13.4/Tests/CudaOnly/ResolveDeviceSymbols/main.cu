
#include <iostream>

#include "file1.h"
#include "file2.h"

int file2_launch_kernel(int x);

result_type_dynamic __device__ file2_func(int x);
static __global__ void main_kernel(result_type_dynamic& r, int x)
{
  // call function that was not device linked to us, this will cause
  // a runtime failure of "invalid device function"
  r = file2_func(x);
}

int main_launch_kernel(int x)
{
  result_type_dynamic r;
  main_kernel<<<1, 1>>>(r, x);
  return r.sum;
}

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
    std::cout << "prop.major: " << prop.major << std::endl;
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

  main_launch_kernel(1);
  cudaError_t err = cudaGetLastError();
  if (err == cudaSuccess) {
    // This kernel launch should fail as the file2_func was device linked
    // into the static library and is not usable by the executable
    std::cerr << "main_launch_kernel: kernel launch should have failed"
              << std::endl;
    return 1;
  }

  return 0;
}
