cmake_minimum_required(VERSION 3.7)
project(RetrieveDependencies C)

add_library(a a.c)

add_library(b c.c)
target_link_libraries(a b)

add_library(c c.c)
add_dependencies(a c)

get_property(DEPS_A TARGET a PROPERTY MANUALLY_ADDED_DEPENDENCIES)

if(NOT DEPS_A STREQUAL "c")
  message(FATAL_ERROR "Expected target c being a dependency of a but got: '${DEPS_A}'")
endif()
