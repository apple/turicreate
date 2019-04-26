cmake_minimum_required(VERSION 3.7)
project(MixedStandardLevels CXX CUDA)

string(APPEND CMAKE_CUDA_FLAGS " -gencode arch=compute_30,code=compute_30")

set(CMAKE_CXX_STANDARD 11)

add_executable(MixedStandardLevels main.cu)
target_compile_features(MixedStandardLevels PUBLIC cxx_std_11)

if(APPLE)
  # Help the static cuda runtime find the driver (libcuda.dyllib) at runtime.
  set_property(TARGET MixedStandardLevels PROPERTY BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
endif()
