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
  "1000 -12*5"
  "1000 +12*-5"
  "1000 -12*-5"
  )

set(FILE_EXPRESSIONS "extern void test_expression(int x, int y, const char * text);\n")


macro(add_math_test expression)
  math(EXPR result ${expression} ${ARGV1} ${ARGV2})
  set(CODE "test_expression(${expression}, ${result}, \"${expression}\");")
  string(APPEND FILE_EXPRESSIONS "${CODE}\n")
endmacro()

macro(add_math_tests)
  foreach (expression ${expressions})
    add_math_test(${expression} ${ARGV0} ${ARGV1})
  endforeach ()
endmacro()

add_math_tests()
add_math_tests("OUTPUT_FORMAT" "DECIMAL")
add_math_tests("OUTPUT_FORMAT" "HEXADECIMAL")

# Avoid the test with negative result and hexadecimal formatting
# therefore more tests with a negative result
add_math_test("-12*5")
add_math_test("12*-5")


configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/MathTestTests.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/MathTestTests.h"
  @ONLY)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
add_executable(MathTest MathTestExec.cxx)
