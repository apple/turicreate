cmake_minimum_required(VERSION 3.10)
project(MacOsFW)
include("../AutogenTest.cmake")

find_package(Qt5Test REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/lib)
set(CMAKE_INSTALL_NAME_DIR ${CMAKE_BINARY_DIR}/output/lib)

if(POLICY CMP0042) # in CMake 3.0.0+
  set (CMAKE_MACOSX_RPATH OFF) # otherwise ON by default
endif(POLICY CMP0042)

if(POLICY CMP0068) # in CMake 3.9+
  cmake_policy(SET CMP0068 NEW)
endif(POLICY CMP0068)

add_subdirectory(src)
add_subdirectory(test)
