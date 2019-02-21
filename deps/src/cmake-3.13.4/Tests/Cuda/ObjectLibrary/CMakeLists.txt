cmake_minimum_required(VERSION 3.7)
project (ObjectLibrary CUDA CXX)
#Goal for this example:
#
#Build C++ and CUDA object files and than use them to make an executable
#Make sure that CMake logic to handle object output when multiple files
#with the same name works
add_subdirectory(Conflicts)

add_library(CudaMixedObjectLib OBJECT static.cu static.cpp)

add_executable(CudaObjectLibrary
               main.cpp
               $<TARGET_OBJECTS:CudaMixedObjectLib>
               $<TARGET_OBJECTS:CudaConflicts>)

if(APPLE)
  # Help the static cuda runtime find the driver (libcuda.dyllib) at runtime.
  set_property(TARGET CudaObjectLibrary PROPERTY BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
endif()
