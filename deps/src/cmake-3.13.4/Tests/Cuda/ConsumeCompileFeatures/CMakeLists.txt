
cmake_minimum_required(VERSION 3.7)
project (ConsumeCompileFeatures CXX CUDA)
#Goal for this example:

#build a c++11 library that express a c++11 public compile feature
#link a cuda library and verify it builds with c++11 enabled

#build a standalone c++/cuda mixed executable where we express a c++11
#compile feature.


add_library(CudaConsumeLib STATIC static.cpp static.cu)
target_compile_features(CudaConsumeLib PUBLIC cxx_nullptr)

add_executable(CudaConsumeCompileFeatures main.cu)
target_link_libraries(CudaConsumeCompileFeatures PRIVATE CudaConsumeLib)
