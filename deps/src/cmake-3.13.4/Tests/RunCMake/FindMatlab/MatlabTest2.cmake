cmake_minimum_required (VERSION 2.8.12)
enable_testing()
project(findmatlab_runcmake_test2)

if(NOT DEFINED matlab_required)
  set(matlab_required REQUIRED)
endif()

if(NOT DEFINED Matlab_ROOT_DIR AND NOT "${matlab_root}" STREQUAL "")
  set(Matlab_ROOT_DIR ${matlab_root})
endif()

find_package(Matlab ${matlab_required} COMPONENTS MX_LIBRARY)
