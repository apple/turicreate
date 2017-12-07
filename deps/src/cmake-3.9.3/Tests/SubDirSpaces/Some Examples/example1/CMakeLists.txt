cmake_minimum_required (VERSION 2.6)
project(example1)
add_executable(example1 example1.cxx)

add_custom_command(TARGET example1 POST_BUILD
  COMMAND "${CMAKE_COMMAND}" ARGS -E remove ${SUBDIR_BINARY_DIR}/ShouldBeHere
  COMMENT "Remove marker file that should exist because this should not be run")
