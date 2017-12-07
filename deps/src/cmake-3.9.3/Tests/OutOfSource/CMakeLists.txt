# a simple test cas
cmake_minimum_required (VERSION 2.6)
project (OutOfSource)

add_subdirectory(SubDir)

get_directory_property(ANIMAL DIRECTORY OutOfSourceSubdir DEFINITION WEASELS)
get_directory_property(ANIMALREL DIRECTORY SubDir/../OutOfSourceSubdir DEFINITION WEASELS)
if(NOT "${ANIMAL}" STREQUAL "${ANIMALREL}")
  message(FATAL_ERROR "GET_DIRECTORY_PROPERTY does not seem to collapse paths.")
endif()

configure_file(
  ${OutOfSource_SOURCE_DIR}/testdp.h.in
  ${OutOfSource_BINARY_DIR}/SubDir/OutOfSourceSubdir/testdp.h
  )

set(KEN 1)
