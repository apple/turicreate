
#include <cublas_v2.h>
#include <cuda_runtime.h>
#include <iostream>

#if defined(USE_THREADS_POSIX) && defined(HAVE_PTHREAD_H)

#  include <pthread.h>
static int verify_linking_to_pthread()
{
  return static_cast<int>(pthread_self());
}
#endif

// this test only makes sense for versions of CUDA that ships
// static libraries that have separable compilation device symbols
#if __CUDACC_VER_MAJOR__ <= 9
__global__ void deviceCublasSgemm(int n, float alpha, float beta,
                                  const float* d_A, const float* d_B,
                                  float* d_C)
{
  cublasHandle_t cnpHandle;
  cublasStatus_t status = cublasCreate(&cnpHandle);

  if (status != CUBLAS_STATUS_SUCCESS) {
    return;
  }

  // Call function defined in the cublas_device system static library.
  // This way we can verify that we properly pass system libraries to the
  // device link line
  status = cublasSgemm(cnpHandle, CUBLAS_OP_N, CUBLAS_OP_N, n, n, n, &alpha,
                       d_A, n, d_B, n, &beta, d_C, n);

  cublasDestroy(cnpHandle);
}
#endif

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

    if (prop.major > 3 || (prop.major == 3 && prop.minor >= 5)) {
      err = cudaSetDevice(i);
      if (err != cudaSuccess) {
        std::cout << "Could not select CUDA device " << i << std::endl;
      } else {
        return 0;
      }
    }
  }

  std::cout << "Could not find a CUDA enabled card supporting compute >=3.5"
            << std::endl;
  return 1;
}

int main(int argc, char** argv)
{
  int ret = choose_cuda_device();
  if (ret) {
    return 0;
  }

#if __CUDACC_VER_MAJOR__ <= 9
  // initial values that will make sure that the cublasSgemm won't actually
  // do any work
  int n = 0;
  float alpha = 1;
  float beta = 1;
  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  deviceCublasSgemm<<<1, 1>>>(n, alpha, beta, d_A, d_B, d_C);
#endif

  return 0;
}
