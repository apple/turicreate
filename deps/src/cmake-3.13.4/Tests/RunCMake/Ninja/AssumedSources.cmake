cmake_minimum_required(VERSION 3.8)
project(AssumedSources)

set_source_files_properties(
  "${CMAKE_CURRENT_BINARY_DIR}/target.c"
  "${CMAKE_CURRENT_BINARY_DIR}/target-no-depends.c"
  PROPERTIES GENERATED 1)

add_executable(working
  "${CMAKE_CURRENT_BINARY_DIR}/target.c"
  "${CMAKE_CURRENT_BINARY_DIR}/target-no-depends.c")

add_custom_target(
  gen-target.c ALL
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/dep.c" "${CMAKE_CURRENT_BINARY_DIR}/target.c")
add_custom_target(
  gen-target-no-depends.c ALL
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/dep.c" "${CMAKE_CURRENT_BINARY_DIR}/target-no-depends.c")

add_dependencies(working gen-target.c)
