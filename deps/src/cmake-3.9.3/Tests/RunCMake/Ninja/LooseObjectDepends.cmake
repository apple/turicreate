cmake_minimum_required(VERSION 3.8)
project(LooseObjectDepends C)

add_custom_command(
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/command.h"
  COMMAND "${CMAKE_COMMAND}" -E touch
          "${CMAKE_CURRENT_BINARY_DIR}/command.h"
          COMMENT "Creating command.h")
add_custom_target(create-command.h
  DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/command.h")

add_custom_target(create-target.h
  BYPRODUCTS  "${CMAKE_CURRENT_BINARY_DIR}/target.h"
  COMMAND "${CMAKE_COMMAND}" -E touch
          "${CMAKE_CURRENT_BINARY_DIR}/target.h"
  COMMENT "Creating target.h")

add_library(dep SHARED dep.c)
add_dependencies(dep create-command.h create-target.h)
target_include_directories(dep
  PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}")

add_library(top top.c)
target_link_libraries(top PRIVATE dep)
