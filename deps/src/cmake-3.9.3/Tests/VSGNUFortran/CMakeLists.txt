cmake_minimum_required(VERSION 2.8)
project(VSGNUFortran)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

# force the executable to be put out of Debug/Release dir
# because gmake build of fortran will not be in a config
# directory, and for easier testing we want the exe and .dll
# to be in the same directory.
if(CMAKE_CONFIGURATION_TYPES)
  foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${config}" config)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config}
      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  endforeach()
endif()

add_subdirectory(subdir)
include_directories(${VSGNUFortran_BINARY_DIR}/subdir/fortran)
add_subdirectory(c_code)
# use a cmake script to run the executable so that PATH
# can be set with the MinGW/bin in it, and the fortran
# runtime libraries can be found.
configure_file(runtest.cmake.in runtest.cmake @ONLY)
