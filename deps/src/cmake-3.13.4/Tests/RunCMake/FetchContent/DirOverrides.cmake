include(FetchContent)

# Test using saved details
FetchContent_Declare(
  t1
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/savedSrc
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E make_directory <SOURCE_DIR>
)
FetchContent_Populate(t1)
if(NOT IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/savedSrc)
  message(FATAL_ERROR "Saved details SOURCE_DIR override failed")
endif()

# Test direct population
FetchContent_Populate(
  t2
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/directSrc
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E make_directory <SOURCE_DIR>
)
if(NOT IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/directSrc)
  message(FATAL_ERROR "Direct details SOURCE_DIR override failed")
endif()

# Ensure setting BINARY_DIR to SOURCE_DIR works (a technique to
# prevent an unwanted separate BINARY_DIR from being created, which
# ExternalProject_Add() does whether we like it or not)
FetchContent_Declare(
  t3
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/savedNoBuildDir
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/savedNoBuildDir
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E make_directory <SOURCE_DIR>
)
FetchContent_Populate(t3)
if(IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/savedNobuildDir-build)
  message(FATAL_ERROR "Saved details BINARY_DIR override failed")
endif()

FetchContent_Populate(
  t4
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/directNoBuildDir
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/directNoBuildDir
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E make_directory <SOURCE_DIR>
)
if(IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/savedNobuildDir-build)
  message(FATAL_ERROR "Direct details BINARY_DIR override failed")
endif()
