
cmake_minimum_required(VERSION 3.7)
project (CudaOnlySeparateCompilation CUDA)

#Goal for this example:
#Build a static library that defines multiple methods and kernels that
#use each other.
#After that confirm that we can call those methods from dynamic libraries
#and executables.
#We complicate the matter by also testing that multiple static libraries
#all containing cuda separable compilation code links properly
string(APPEND CMAKE_CUDA_FLAGS " -gencode arch=compute_30,code=compute_30")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CUDA_STANDARD 11)

add_library(CUDASeparateLibA STATIC file1.cu file2.cu file3.cu)

if(CMAKE_CUDA_SIMULATE_ID STREQUAL "MSVC")
  # Test adding a flag that is not in our CUDA flag table for VS.
  if(NOT CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 8)
    string(APPEND CMAKE_CUDA_FLAGS " --ftemplate-depth 50")
  endif()
  # Test adding a flag that nvcc should pass to the host compiler.
  target_compile_options(CUDASeparateLibA PRIVATE -Xcompiler=-bigobj)
endif()

#Having file4/file5 in a shared library causes serious problems
#with the nvcc linker and it will generate bad entries that will
#cause a segv when trying to run the executable
#
add_library(CUDASeparateLibB STATIC file4.cu file5.cu)
target_link_libraries(CUDASeparateLibB PRIVATE CUDASeparateLibA)

add_executable(CudaOnlySeparateCompilation main.cu)
target_link_libraries(CudaOnlySeparateCompilation
                      PRIVATE CUDASeparateLibB)

set_target_properties(CUDASeparateLibA
                      CUDASeparateLibB
                      PROPERTIES CUDA_SEPARABLE_COMPILATION ON
                      POSITION_INDEPENDENT_CODE ON)

if (CMAKE_GENERATOR MATCHES "^Visual Studio")
  #Visual Studio CUDA integration will not perform device linking
  #on a target that itself does not have GenerateRelocatableDeviceCode
  #enabled.
  set_target_properties(CudaOnlySeparateCompilation
                        PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif()

if (APPLE)
  # We need to add the default path to the driver (libcuda.dylib) as an rpath, so that
  # the static cuda runtime can find it at runtime.
  target_link_libraries(CudaOnlySeparateCompilation PRIVATE -Wl,-rpath,/usr/local/cuda/lib)
endif()
