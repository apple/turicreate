cmake_minimum_required(VERSION 3.2)
project(ExternalProjectSubdir NONE)
include(ExternalProject)

# Remove the custom target output to be sure it runs in an
# incremental test.
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/PreSubdir1.txt)

add_custom_target(PreSubdir1
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/PreSubdir1.txt
  )
add_library(PreSubdir1Interface INTERFACE)
add_dependencies(PreSubdir1Interface PreSubdir1)

ExternalProject_Add(Subdir1
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Subdir1
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Subdir1

  CMAKE_ARGS -DNORMAL_VAR=NORMAL_VALUE -DGENEX_VAR=$<1:GENEX_VALUE>
  LOG_CONFIGURE 1

  BUILD_COMMAND ""
  INSTALL_COMMAND ""

  DEPENDS PreSubdir1Interface
  )
