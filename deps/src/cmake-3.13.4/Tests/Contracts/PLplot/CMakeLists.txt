cmake_minimum_required(VERSION 3.9)
project(PLplotDriver NONE)
include(ExternalProject)
include(${CMAKE_CURRENT_SOURCE_DIR}/../Home.cmake)
set(PLplot_PREFIX "${HOME}/.cmake/Contracts/PLplot")
file(REMOVE_RECURSE "${PLplot_PREFIX}")
separate_arguments(PLplot_CMAKE_ARGS UNIX_COMMAND "${PLplot_CMAKE_FLAGS}")
if(NOT PLplot_GIT_TAG)
  set(PLplot_GIT_TAG "plplot-5.13.0")
endif()
ExternalProject_Add(PLplot
  GIT_REPOSITORY "https://git.code.sf.net/p/plplot/plplot"
  GIT_TAG "${PLplot_GIT_TAG}"
  PREFIX "${PLplot_PREFIX}"
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    ${PLplot_CMAKE_ARGS}
  )
