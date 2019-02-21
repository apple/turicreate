cmake_minimum_required (VERSION 2.6)
project(Object)
if(APPLE)
  # set _CMAKE_OSX_MACHINE to umame -m
  exec_program(uname ARGS -m OUTPUT_VARIABLE _CMAKE_OSX_MACHINE)
  # check for Power PC and change to ppc
  if("${_CMAKE_OSX_MACHINE}" MATCHES "Power")
    set(_CMAKE_OSX_MACHINE ppc)
  endif()
  set(CMAKE_OSX_ARCHITECTURES ${_CMAKE_OSX_MACHINE})
endif()

add_executable(external external_object.cxx external_main.cxx)
