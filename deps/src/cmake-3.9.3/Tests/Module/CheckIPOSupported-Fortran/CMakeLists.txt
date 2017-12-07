cmake_minimum_required(VERSION 3.8)
project(CheckIPOSupported-Fortran LANGUAGES Fortran)

cmake_policy(SET CMP0069 NEW)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
elseif(CMake_TEST_IPO_WORKS_Fortran)
  message(FATAL_ERROR "IPO expected to work")
endif()

add_library(foo foo.f)
add_executable(CheckIPOSupported-Fortran main.f)
target_link_libraries(CheckIPOSupported-Fortran PUBLIC foo)

enable_testing()
add_test(NAME CheckIPOSupported-Fortran COMMAND CheckIPOSupported-Fortran)
