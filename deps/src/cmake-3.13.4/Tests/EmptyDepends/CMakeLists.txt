cmake_minimum_required(VERSION 2.8)
project(EmptyDepends)

include(CTest)

set(extra_dep)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/qrc_my.cxx
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_BINARY_DIR}/qrc_my.cxx"
  DEPENDS "${extra_dep}" "${CMAKE_BINARY_DIR}/my.qrc")

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/my.qrc
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/my.qrc)

add_library(qrc STATIC ${CMAKE_BINARY_DIR}/qrc_my.cxx)
