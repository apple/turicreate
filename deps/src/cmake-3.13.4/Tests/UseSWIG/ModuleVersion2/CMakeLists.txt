cmake_minimum_required(VERSION 3.1...3.13)

project(TestModuleVersion2 CXX)

include(CTest)

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

find_package(Python2 REQUIRED COMPONENTS Interpreter Development)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

if (WIN32)
  set (PS $<SEMICOLON>)
else()
  set (PS ":")
endif()

set (UseSWIG_MODULE_VERSION 2)
unset(CMAKE_SWIG_FLAGS)

set_property(SOURCE "../example.i" PROPERTY CPLUSPLUS ON)
set_property(SOURCE "../example.i" PROPERTY COMPILE_OPTIONS -includeall)

set_property(SOURCE "../example.i"
  PROPERTY GENERATED_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/..")

swig_add_library(example1
                 LANGUAGE python
                 SOURCES ../example.i ../example.cxx)
set_target_properties (example1 PROPERTIES
  OUTPUT_NAME example
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python2"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python2"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python2")
target_link_libraries(example1 PRIVATE Python2::Python)

# re-use sample interface file for another plugin
swig_add_library(example2
                 LANGUAGE python
                 SOURCES ../example.i ../example.cxx)
set_target_properties (example2 PROPERTIES
  OUTPUT_NAME example
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python3"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python3"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python3")
target_link_libraries(example2 PRIVATE Python3::Python)


add_test (NAME ModuleVersion2.example1
  COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=$<TARGET_PROPERTY:example1,SWIG_SUPPORT_FILES_DIRECTORY>${PS}$<TARGET_FILE_DIR:example1>"
  "${Python2_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../runme.py")

add_test (NAME ModuleVersion2.example2
  COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=$<TARGET_PROPERTY:example2,SWIG_SUPPORT_FILES_DIRECTORY>${PS}$<TARGET_FILE_DIR:example2>"
  "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../runme.py")
