cmake_minimum_required(VERSION 3.1...3.13)

project(TestMultipleModules CXX)

include(CTest)

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

find_package(Python REQUIRED COMPONENTS Interpreter Development)

find_package(Perl REQUIRED)
find_package(PerlLibs REQUIRED)

# Path separator
if (WIN32)
  set (PS "$<SEMICOLON>")
else()
  set (PS ":")
endif()

unset(CMAKE_SWIG_FLAGS)

set_property(SOURCE "../example.i" PROPERTY CPLUSPLUS ON)
set_property(SOURCE "../example.i" PROPERTY COMPILE_OPTIONS -includeall)

set_property(SOURCE "../example.i"
  PROPERTY GENERATED_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/..")

swig_add_library(example1
                 LANGUAGE python
                 OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Python"
                 SOURCES ../example.i ../example.cxx)
set_target_properties (example1 PROPERTIES
  OUTPUT_NAME example
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python")
target_link_libraries(example1 PRIVATE Python::Python)

# re-use sample interface file for another plugin
set_property(SOURCE "../example.i" APPEND PROPERTY
  GENERATED_INCLUDE_DIRECTORIES ${PERL_INCLUDE_PATH})
separate_arguments(c_flags UNIX_COMMAND "${PERL_EXTRA_C_FLAGS}")
set_property(SOURCE "../example.i" PROPERTY GENERATED_COMPILE_OPTIONS ${c_flags})

swig_add_library(example2
                 LANGUAGE perl
                 OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Perl"
                 SOURCES ../example.i ../example.cxx)
set_target_properties (example2 PROPERTIES
  OUTPUT_NAME example
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Perl"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Perl"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Perl")
target_link_libraries(example2 PRIVATE ${PERL_LIBRARY})



add_test (NAME MultipleModules.Python
  COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/Python${PS}$<TARGET_FILE_DIR:example1>"
  "${Python_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../runme.py")


add_test (NAME MultipleModules.Perl
  COMMAND "${PERL_EXECUTABLE}" "-I${CMAKE_CURRENT_BINARY_DIR}/Perl"
                               "-I$<TARGET_FILE_DIR:example2>"
  "${CMAKE_CURRENT_SOURCE_DIR}/../runme.pl")
