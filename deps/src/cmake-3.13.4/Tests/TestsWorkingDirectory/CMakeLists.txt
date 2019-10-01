cmake_minimum_required(VERSION 2.6)
project(TestsWorkingDirectoryProj)

add_executable(WorkingDirectory main.c)

enable_testing()

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

add_test(NAME WorkingDirectory0 COMMAND WorkingDirectory "${CMAKE_BINARY_DIR}")

add_test(NAME WorkingDirectory1 COMMAND WorkingDirectory "${CMAKE_BINARY_DIR}")
set_tests_properties(WorkingDirectory1 PROPERTIES
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

string(REGEX REPLACE "/[^/]*$" "" _parent_dir "${CMAKE_BINARY_DIR}")

add_test(NAME WorkingDirectory2 COMMAND WorkingDirectory "${_parent_dir}")
set_tests_properties(WorkingDirectory2 PROPERTIES
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/.."
)

set(_default_cwd "${CMAKE_BINARY_DIR}")

# FIXME: How to deal with /debug, /release, etc. with VS or Xcode?
if(${CMAKE_GENERATOR} MATCHES "Makefiles")
add_test(WorkingDirectory3 ${EXECUTABLE_OUTPUT_PATH}/WorkingDirectory ${_default_cwd})
endif()

add_test(NAME WorkingDirectory4 WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND WorkingDirectory ${CMAKE_BINARY_DIR})

string(REGEX REPLACE "/[^/]*$" "" _parent_dir "${CMAKE_BINARY_DIR}")

add_test(NAME WorkingDirectory5 WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/.. COMMAND WorkingDirectory ${_parent_dir})

# FIXME: How to deal with /debug, /release, etc. with VS or Xcode?
if(${CMAKE_GENERATOR} MATCHES "Makefiles")
add_test(WorkingDirectory6 ${EXECUTABLE_OUTPUT_PATH}/WorkingDirectory ${_default_cwd} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..)
endif()

add_subdirectory(subdir)
