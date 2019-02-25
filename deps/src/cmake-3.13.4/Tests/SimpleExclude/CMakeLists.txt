cmake_minimum_required (VERSION 2.6)
project(SimpleExclude C)

set(EXECUTABLE_OUTPUT_PATH "${SimpleExclude_BINARY_DIR}" CACHE INTERNAL "" FORCE)
set(LIBRARY_OUTPUT_PATH "${SimpleExclude_BINARY_DIR}" CACHE INTERNAL "" FORCE)

add_subdirectory(dirC EXCLUDE_FROM_ALL)
add_subdirectory(dirD)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/run.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/run.cmake" @ONLY)
