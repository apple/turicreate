cmake_minimum_required(VERSION 3.7)
project (CudaOnlyResolveDeviceSymbols CUDA)

# Find nm and dumpbin
if(CMAKE_NM)
  set(dump_command ${CMAKE_NM})
  set(dump_args -g)
else()
  include(GetPrerequisites)
  message(STATUS "calling list_prerequisites to find dumpbin")
  list_prerequisites("${CMAKE_COMMAND}" 0 0 0)
  if(gp_dumpbin)
    set(dump_command ${gp_dumpbin})
    set(dump_args /ARCHIVEMEMBERS)
  endif()
endif()

#Goal for this example:
#Build a static library that defines multiple methods and kernels that
#use each other.
#Use a custom command to build an executable that uses this static library
#We do these together to verify that we can get a static library to do
#device symbol linking, and not have it done when the executable is made
string(APPEND CMAKE_CUDA_FLAGS " -gencode arch=compute_30,code=compute_30")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CUDA_STANDARD 11)

add_library(CUDAResolveDeviceLib STATIC file1.cu file2.cu)
set_target_properties(CUDAResolveDeviceLib
                      PROPERTIES
                      CUDA_SEPARABLE_COMPILATION ON
                      CUDA_RESOLVE_DEVICE_SYMBOLS ON
                      POSITION_INDEPENDENT_CODE ON)

if(dump_command)
add_custom_command(TARGET CUDAResolveDeviceLib POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -DDUMP_COMMAND=${dump_command}
  -DDUMP_ARGS=${dump_args}
  -DTEST_LIBRARY_PATH=$<TARGET_FILE:CUDAResolveDeviceLib>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/verify.cmake
  )
endif()

add_executable(CudaOnlyResolveDeviceSymbols main.cu)
target_link_libraries(CudaOnlyResolveDeviceSymbols PRIVATE CUDAResolveDeviceLib)

if(APPLE)
  # We need to add the default path to the driver (libcuda.dylib) as an rpath, so that
  # the static cuda runtime can find it at runtime.
  target_link_libraries(CudaOnlyResolveDeviceSymbols PRIVATE -Wl,-rpath,/usr/local/cuda/lib)
endif()
