
cmake_minimum_required(VERSION 3.7)
project (CudaComplex CXX CUDA)
#Goal for this example:

#build a cpp dynamic library base
#build a cuda static library base that uses separable compilation

#build a cuda dynamic library that uses the first dynamic library
#build a mixed cpp & cuda dynamic library uses all 3 previous libraries

#lastly build a cpp executable that uses this last cuda dynamic library

#this tests that we can properly handle linking cuda and cpp together
#and also bulding cpp targets that need cuda implicit libraries

#verify that we can pass explicit cuda arch flags
string(APPEND CMAKE_CUDA_FLAGS " -gencode arch=compute_30,code=compute_30")
set(CMAKE_CUDA_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CUDA_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(CudaComplexCppBase SHARED dynamic.cpp)
add_library(CudaComplexSeperableLib STATIC file1.cu file2.cu file3.cu)
set_target_properties(CudaComplexSeperableLib
                       PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
set_target_properties( CudaComplexSeperableLib
                       PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(CudaComplexSharedLib SHARED dynamic.cu)
target_link_libraries(CudaComplexSharedLib PUBLIC CudaComplexCppBase)

add_library(CudaComplexMixedLib SHARED mixed.cpp mixed.cu)
set_target_properties(CudaComplexMixedLib
                       PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_link_libraries(CudaComplexMixedLib
                      PUBLIC CudaComplexSharedLib
                      PRIVATE CudaComplexSeperableLib)

add_executable(CudaComplex main.cpp)
target_link_libraries(CudaComplex PUBLIC CudaComplexMixedLib)

if(APPLE)
  # We need to add the default path to the driver (libcuda.dylib) as an rpath, so that
  # the static cuda runtime can find it at runtime.
  target_link_libraries(CudaComplex PRIVATE -Wl,-rpath,/usr/local/cuda/lib)
endif()
