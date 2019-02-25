cmake_minimum_required(VERSION 2.8)
project(LinkDirectory C)

# Put the subproject source tree in our build tree so it can refer to
# link directories relative to its source.
if(NOT "${LinkDirectory_SOURCE_DIR}" STREQUAL "${LinkDirectory_BINARY_DIR}")
  file(COPY External/ DESTINATION External PATTERN CVS EXCLUDE)
endif()

# Build a library into the subproject source tree.
add_library(mylibA STATIC mylibA.c)
set_property(TARGET mylibA PROPERTY
  ARCHIVE_OUTPUT_DIRECTORY "${LinkDirectory_BINARY_DIR}/External/lib")

# Build a library into our build tree relative to the subproject build tree.
add_library(mylibB STATIC mylibB.c)
set_property(TARGET mylibB PROPERTY
  ARCHIVE_OUTPUT_DIRECTORY "${LinkDirectory_BINARY_DIR}/lib")

# Create a custom target to drive the subproject build.
include(ExternalProject)
ExternalProject_Add(ExternalTarget
  SOURCE_DIR "${LinkDirectory_BINARY_DIR}/External"
  BINARY_DIR "${LinkDirectory_BINARY_DIR}/External-build"
  CMAKE_ARGS "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${LinkDirectory_BINARY_DIR}/bin"
  PREFIX "${LinkDirectory_BINARY_DIR}/External-build/root"
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
  )

# Add a step to wipe out the subproject executable after our libraries
# change.  This is needed because the subproject cannot depend on them
# directly because it does not know the full paths to the libraries.
# (The purpose of this test is to check that link_directories works.)
ExternalProject_Add_Step(ExternalTarget cleanup
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${LinkDirectory_BINARY_DIR}/bin
  DEPENDEES download
  DEPENDERS configure
  DEPENDS mylibA mylibB
    "${LinkDirectory_BINARY_DIR}/External/CMakeLists.txt"
    "${LinkDirectory_BINARY_DIR}/External/myexe.c"
  )

# Make the subproject build after our targets.
add_dependencies(ExternalTarget mylibA mylibB)
