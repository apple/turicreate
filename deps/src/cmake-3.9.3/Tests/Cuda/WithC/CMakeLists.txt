cmake_minimum_required(VERSION 3.7)
project(CudaComplex CUDA C)

string(APPEND CMAKE_CUDA_FLAGS " -gencode arch=compute_30,code=compute_30")

add_executable(CudaWithC main.c cuda.cu)

if(APPLE)
  # We need to add the default path to the driver (libcuda.dylib) as an rpath, so that
  # the static cuda runtime can find it at runtime.
  target_link_libraries(CudaWithC PRIVATE -Wl,-rpath,/usr/local/cuda/lib)
endif()
