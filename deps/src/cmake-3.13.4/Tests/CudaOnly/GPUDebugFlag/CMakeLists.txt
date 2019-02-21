
cmake_minimum_required(VERSION 3.7)
project (GPUDebugFlag CUDA)

#Goal for this example:
#verify that -G enables gpu debug flags
string(APPEND CMAKE_CUDA_FLAGS " -gencode=arch=compute_30,code=compute_30")
string(APPEND CMAKE_CUDA_FLAGS " -G")
set(CMAKE_CUDA_STANDARD 11)

add_executable(CudaOnlyGPUDebugFlag main.cu)

if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 9.0.0)
  #CUDA's __CUDACC_DEBUG__ define was added in 9.0
  #so if we are below 9.0.0 we will manually add the define so that the test
  #passes
  target_compile_definitions(CudaOnlyGPUDebugFlag PRIVATE "__CUDACC_DEBUG__")
endif()

if(APPLE)
  # Help the static cuda runtime find the driver (libcuda.dyllib) at runtime.
  set_property(TARGET CudaOnlyGPUDebugFlag PROPERTY BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
endif()
