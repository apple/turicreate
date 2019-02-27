cmake_minimum_required(VERSION 3.7)
project (CircularLinkLine CUDA)

#Goal for this example:
# Verify that we de-duplicate the device link line
# Verify that a de-duplicated link line still works with circular static libraries

string(APPEND CMAKE_CUDA_FLAGS " -gencode arch=compute_30,code=[compute_30]")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CUDA_STANDARD 11)

add_library(CUDACircularDeviceLinking1 STATIC file1.cu)
add_library(CUDACircularDeviceLinking2 STATIC file2.cu)
add_library(CUDACircularDeviceLinking3 STATIC file3.cu)
add_executable(CudaOnlyCircularLinkLine main.cu)

target_link_libraries(CUDACircularDeviceLinking1 PUBLIC CUDACircularDeviceLinking2)
target_link_libraries(CUDACircularDeviceLinking2 PUBLIC CUDACircularDeviceLinking3)
target_link_libraries(CUDACircularDeviceLinking3 PUBLIC CUDACircularDeviceLinking1)

target_link_libraries(CudaOnlyCircularLinkLine PRIVATE CUDACircularDeviceLinking3)


set_target_properties(CUDACircularDeviceLinking1
                      PROPERTIES
                      CUDA_SEPARABLE_COMPILATION ON)

set_target_properties(CUDACircularDeviceLinking2
                      PROPERTIES
                      CUDA_SEPARABLE_COMPILATION ON)

set_target_properties(CUDACircularDeviceLinking3
                      PROPERTIES
                      CUDA_SEPARABLE_COMPILATION ON)
