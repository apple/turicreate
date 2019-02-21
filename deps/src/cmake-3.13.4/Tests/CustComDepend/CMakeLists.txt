cmake_minimum_required (VERSION 2.6)
project(CustComDepend)
include_directories("${CustComDepend_SOURCE_DIR}")
add_definitions(-D_CRT_SECURE_NO_DEPRECATE=1)
set(EXECUTABLE_OUTPUT_PATH ${CustComDepend_BINARY_DIR}/bin)
add_executable(foo foo.cxx)
add_custom_command(
  OUTPUT ${CustComDepend_BINARY_DIR}/bar.c
  COMMAND ${CustComDepend_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/foo
  ${CustComDepend_BINARY_DIR}/bar.c
  DEPENDS ${CustComDepend_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/foo
)

add_library(bar SHARED ${CustComDepend_BINARY_DIR}/bar.c)
