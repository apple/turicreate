# This is the canonical simplest ExternalProject example CMakeLists.txt file:
cmake_minimum_required(VERSION 2.8)
project(ExternalProjectExample NONE)
include(ExternalProject)

ExternalProject_Add(
  cmake281
  URL https://cmake.org/files/v2.8/cmake-2.8.1.tar.gz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  BUILD_COMMAND ""
)
