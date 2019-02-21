cmake_minimum_required(VERSION 2.8.12)
project(CTestTestSkipReturnCode)
include(CTest)

add_test (NAME CMakeV1 COMMAND ${CMAKE_COMMAND} "--version")
add_test (NAME CMakeV2 COMMAND ${CMAKE_COMMAND} "--version")

set_tests_properties(CMakeV2 PROPERTIES SKIP_RETURN_CODE 0)
