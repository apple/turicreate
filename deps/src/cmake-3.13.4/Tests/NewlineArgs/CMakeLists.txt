# a simple CXX only test case
cmake_minimum_required (VERSION 2.6)
project (NewlineArgs CXX)

add_definitions("-DTEST_FLAG_1
-DTEST_FLAG_2")

include_directories(" ${NewlineArgs_BINARY_DIR}
 ${NewlineArgs_SOURCE_DIR} ")

configure_file("${NewlineArgs_SOURCE_DIR}/libcxx2.h.in"
  "${NewlineArgs_BINARY_DIR}/libcxx2.h")

add_library(testcxx1 libcxx1.cxx)
add_executable (NewlineArgs cxxonly.cxx)
target_link_libraries(NewlineArgs testcxx1)
