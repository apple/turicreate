
cmake_minimum_required(VERSION 2.8)

project(PositionIndependentTargets)

include(CheckCXXSourceCompiles)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}") # For pic_test.h

add_subdirectory(global)
add_subdirectory(targets)
add_subdirectory(interface)

add_executable(PositionIndependentTargets main.cpp)
