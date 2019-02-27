cmake_minimum_required(VERSION 3.13)
project(ProperDeviceLibraries CXX CUDA)

string(APPEND CMAKE_CUDA_FLAGS " -gencode arch=compute_35,code=compute_35 -gencode arch=compute_35,code=sm_35")
set(CMAKE_CUDA_STANDARD 11)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

add_executable(ProperDeviceLibraries main.cu)
set_target_properties(ProperDeviceLibraries
                      PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

add_library(UseThreadsMixed SHARED use_pthreads.cxx use_pthreads.cu)
target_link_libraries(UseThreadsMixed Threads::Threads)

add_library(UseThreadsCuda SHARED use_pthreads.cu)
target_link_libraries(UseThreadsCuda Threads::Threads)

target_link_libraries(ProperDeviceLibraries PRIVATE UseThreadsMixed UseThreadsCuda)

if(THREADS_HAVE_PTHREAD_ARG AND CMAKE_USE_PTHREADS_INIT)
  add_library(UseExplicitPThreadsFlag SHARED use_pthreads.cu)
  target_compile_options(UseExplicitPThreadsFlag PUBLIC "-Xcompiler=-pthread")
  target_link_libraries(UseExplicitPThreadsFlag PUBLIC "-pthread")

  add_library(UseExplicitLThreadsFlag SHARED use_pthreads.cu)
  target_compile_options(UseExplicitLThreadsFlag PUBLIC "-Xcompiler=-pthread")
  target_link_libraries(UseExplicitLThreadsFlag PUBLIC "-lpthread")

  add_library(UseExplicitLongThreadsFlag SHARED use_pthreads.cu)
  target_link_libraries(UseExplicitLongThreadsFlag PUBLIC "--library pthread")

  target_link_libraries(ProperDeviceLibraries PRIVATE UseExplicitPThreadsFlag UseExplicitLThreadsFlag UseExplicitLongThreadsFlag)
endif()

if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 10.0.0)
  #CUDA 10 removed the cublas_device library
  target_link_libraries(ProperDeviceLibraries PRIVATE cublas_device)
endif()

if(APPLE)
  # Help the static cuda runtime find the driver (libcuda.dyllib) at runtime.
  set_property(TARGET ProperDeviceLibraries PROPERTY BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
endif()
