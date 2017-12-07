cmake_minimum_required(VERSION 3.8)
project(CheckIPOSupported-CXX LANGUAGES CXX)

cmake_policy(SET CMP0069 NEW)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
elseif(CMake_TEST_IPO_WORKS_CXX)
  message(FATAL_ERROR "IPO expected to work")
endif()

add_library(foo foo.cpp)
add_executable(CheckIPOSupported-CXX main.cpp)
target_link_libraries(CheckIPOSupported-CXX PUBLIC foo)

enable_testing()
add_test(NAME CheckIPOSupported-CXX COMMAND CheckIPOSupported-CXX)
