cmake_minimum_required(VERSION 3.8)
project (ToolkitInclude CXX CUDA)

#Goal for this example:
# Validate that between the CXX implicit include directories and the
# CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES directories we can find
# the cuda runtime headers

add_executable(CudaToolkitInclude main.cpp)
target_include_directories(CudaToolkitInclude PRIVATE
                           ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
