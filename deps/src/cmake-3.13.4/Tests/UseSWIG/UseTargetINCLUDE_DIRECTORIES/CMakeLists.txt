cmake_minimum_required(VERSION 3.1...3.13)

project(TestUseTargetINCLUDE_DIRECTORIES CXX)

include(CTest)

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

unset(CMAKE_SWIG_FLAGS)

set_property(SOURCE "example.i" PROPERTY CPLUSPLUS ON)
set_property(SOURCE "example.i" PROPERTY COMPILE_OPTIONS -includeall)

swig_add_library(example1
                 LANGUAGE python
                 OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/example1"
                 SOURCES example.i ../example.cxx)
set_target_properties (example1 PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/.."
  SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE
  OUTPUT_NAME example1
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/example1"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/example1"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/example1")
target_link_libraries(example1 PRIVATE Python3::Python)


# Check that source property override target property
set_property(SOURCE "example.i" PROPERTY USE_TARGET_INCLUDE_DIRECTORIES TRUE)

swig_add_library(example2
                 LANGUAGE python
                 OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/example2"
                 SOURCES example.i ../example.cxx)
set_target_properties (example2 PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/.."
  SWIG_USE_TARGET_INCLUDE_DIRECTORIES FALSE
  OUTPUT_NAME example2
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/example2"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/example2"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/example2")
target_link_libraries(example2 PRIVATE Python3::Python)
