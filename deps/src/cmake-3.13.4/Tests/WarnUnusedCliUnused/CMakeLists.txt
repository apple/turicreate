cmake_minimum_required(VERSION 2.8)

project(WarnUnusedCliUnused)

set_directory_properties(PROPERTIES
  ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_BINARY_DIR}/CMakeCache.txt"
)

add_library(dummy empty.cpp)
