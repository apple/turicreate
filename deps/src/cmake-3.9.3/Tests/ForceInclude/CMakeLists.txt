cmake_minimum_required(VERSION 2.8.3.20110103)
project(ForceInclude C)

# Make sure the proper compiler is in use.
if(NOT MSVC AND NOT CMAKE_C_COMPILER_ID STREQUAL "Intel")
  message(FATAL_ERROR "The ForceInclude test works only with MSVC or Intel")
endif()

add_executable(foo foo.c)
set_property(SOURCE foo.c PROPERTY COMPILE_FLAGS "/FIfoo1.h /FIfoo2.h")
