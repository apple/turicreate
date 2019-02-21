cmake_minimum_required (VERSION 2.6)
project(CMAKE_LOADED_COMMANDS)

if (MUDSLIDE_TYPE MATCHES MUCHO)
  add_definitions(-DMUCHO_MUDSLIDE)
endif ()

include_directories(${CMAKE_ROOT}/include ${CMAKE_ROOT}/Source)

add_library(cmCMAKE_TEST_COMMAND MODULE cmTestCommand.c)

if(WATCOM)
  target_link_libraries(cmCMAKE_TEST_COMMAND clbsdll.lib)
endif()
