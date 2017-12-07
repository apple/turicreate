cmake_minimum_required (VERSION 2.6)
project(CTestTestBadExe)
include(CTest)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/notAnExe.txt" "${CMAKE_CURRENT_BINARY_DIR}/notAnExe.txt" COPYONLY)

add_test (TestBadExe "${CMAKE_CURRENT_BINARY_DIR}/notAnExe.txt")
