
cmake_minimum_required(VERSION 3.7)
project (CudaOnlyEnableStandard CUDA)

#Goal for this example:
#build cuda sources that require C++11 to be enabled.

add_library(CUDAStatic11 STATIC static.cu)
add_library(CUDADynamic11 SHARED shared.cu)

add_executable(CudaOnlyEnableStandard main.cu)
target_link_libraries(CudaOnlyEnableStandard PRIVATE CUDAStatic11 CUDADynamic11)

set_target_properties(CUDAStatic11 CUDADynamic11 PROPERTIES CUDA_STANDARD 11)
set_target_properties(CUDAStatic11 CUDADynamic11 PROPERTIES CUDA_STANDARD_REQUIRED TRUE)

#Verify CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES
foreach(dir ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
  if(NOT IS_DIRECTORY "${dir}")
    message(FATAL_ERROR
      "CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES entry\n"
      " ${dir}\n"
      "is not an existing directory."
      )
  endif()
endforeach()
