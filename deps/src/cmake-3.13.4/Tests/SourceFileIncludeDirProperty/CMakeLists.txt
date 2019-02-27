cmake_minimum_required(VERSION 3.0)

project(SourceFileIncludeDirProperty C)

#
# Check that source level include directory take
# precedence over target one

add_executable(SourceFileIncludeDirProperty main.c)

set_property (TARGET SourceFileIncludeDirProperty
  PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/target")

set_property (SOURCE main.c
  PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/source")
