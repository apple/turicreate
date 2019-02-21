cmake_minimum_required(VERSION 3.6)
project(ExternalProjectSourceSubdir NONE)
include(ExternalProject)

ExternalProject_Add(Example
  SOURCE_SUBDIR subdir
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Example
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Example
  INSTALL_COMMAND ""
  )
