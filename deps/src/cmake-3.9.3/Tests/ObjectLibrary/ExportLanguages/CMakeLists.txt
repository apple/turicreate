cmake_minimum_required(VERSION 2.8)
project(ExportLanguages CXX)
add_library(ExportLanguagesA OBJECT a.cxx)
add_library(ExportLanguagesB STATIC a.c $<TARGET_OBJECTS:ExportLanguagesA>)

# Verify that object library languages are propagated.
export(TARGETS ExportLanguagesB NAMESPACE Exp FILE BExport.cmake)
include(ExternalProject)
ExternalProject_Add(ExportLanguagesTest
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ExportLanguagesTest"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/ExportLanguagesTest"
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
)
add_dependencies(ExportLanguagesTest ExportLanguagesA ExportLanguagesB)
