# This is run by test test1 in repeat-until-fail-cmake.cmake with cmake -P.
# It reads the file TEST_OUTPUT_FILE and increments the number
# found in the file by 1.  When the number is 2, then the
# code sends out a cmake error causing the test to fail
# the second time it is run.
message("TEST_OUTPUT_FILE = ${TEST_OUTPUT_FILE}")
file(READ "${TEST_OUTPUT_FILE}" COUNT)
message("COUNT= ${COUNT}")
math(EXPR COUNT "${COUNT} + 1")
file(WRITE "${TEST_OUTPUT_FILE}" "${COUNT}")
if(${COUNT} EQUAL 2)
  message(FATAL_ERROR "this test fails on the 2nd run")
endif()
