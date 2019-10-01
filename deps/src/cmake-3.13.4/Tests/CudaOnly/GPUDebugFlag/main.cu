#include <cuda.h>
#include <cuda_runtime.h>
#include <iostream>

static __global__ void debug_kernel(bool* has_debug)
{
// Verify using the return code if we have GPU debug flag enabled
#if defined(__CUDACC__) && defined(__CUDACC_DEBUG__)
  *has_debug = true;
#else
  *has_debug = false;
#endif
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
  bool* has_debug;
  cudaError_t err = cudaMallocManaged(&has_debug, sizeof(bool));
  if (err != cudaSuccess) {
    std::cerr << "cudaMallocManaged failed:\n"
              << "  " << cudaGetErrorString(err) << std::endl;
    return 1;
  }

  debug_kernel<<<1, 1>>>(has_debug);
  err = cudaDeviceSynchronize();
  if (err != cudaSuccess) {
    std::cerr << "debug_kernel: kernel launch shouldn't have failed\n"
              << "reason:\t" << cudaGetErrorString(err) << std::endl;
    return 1;
  }
  if (*has_debug == false) {
    std::cerr << "debug_kernel: kernel not compiled with device debug"
              << std::endl;
    return 1;
  }
  return 0;
}
