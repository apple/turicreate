cmake_minimum_required (VERSION 2.6)
project(MathTest)


# Expression test

set(expressions
  "5 * ( 3 + 4)"
  "(1 | 2 | 4 | 8) & 16"
  "1 +(3*4) + 10 >> 2"
  "10000 / 20 / 4"
  "10000 / (20 / 4)"
  "-1 + +1"
  "+1 - -1"
  "+1 - - + + -(-3 + - - +1)"
  )

set(FILE_EXPRESSIONS "")
foreach(expression
    ${expressions})
  math(EXPR expr "${expression}")
  string(APPEND FILE_EXPRESSIONS "TEST_EXPRESSION(${expression}, ${expr})\n")
endforeach()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/MathTestTests.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/MathTestTests.h"
  @ONLY)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
add_executable(MathTest MathTestExec.cxx)

