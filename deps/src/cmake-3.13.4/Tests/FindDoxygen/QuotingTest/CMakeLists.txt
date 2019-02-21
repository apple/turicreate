cmake_minimum_required(VERSION 3.10)
project(TestFindDoxygen VERSION 1.0 LANGUAGES NONE)

find_package(Doxygen REQUIRED)

set(DOXYGEN_PROJECT_BRIEF "String with spaces")
set(DOXYGEN_ALIASES
  [[somealias="@some_command param"]]
  "anotherAlias=@foobar"
)

set(DOXYGEN_VERBATIM_VARS DOXYGEN_ALIASES)

doxygen_add_docs(docsQuoting)
if(NOT EXISTS "${PROJECT_BINARY_DIR}/Doxyfile.docsQuoting")
  message(FATAL_ERROR "Missing generated file: Doxyfile.docsQuoting")
endif()

file(STRINGS "${PROJECT_BINARY_DIR}/Doxyfile.docsQuoting" matches
  REGEX [[^PROJECT_BRIEF *= *"String with spaces"]]
)
if(NOT matches)
  message(FATAL_ERROR "PROJECT_BRIEF does not match expected contents")
endif()

file(STRINGS "${PROJECT_BINARY_DIR}/Doxyfile.docsQuoting" matches
  REGEX [[^ALIASES *= *somealias="@some_command param" anotherAlias=@foobar]]
)
if(NOT matches)
  message(FATAL_ERROR "ALIASES does not match expected contents")
endif()

add_custom_target(allDocTargets)
add_dependencies(allDocTargets docsQuoting)
