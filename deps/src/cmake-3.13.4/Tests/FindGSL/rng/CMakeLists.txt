cmake_minimum_required(VERSION 3.0)
project(FindGSL_rng CXX)
include(CTest)

find_package(GSL REQUIRED)

add_executable(tstgslrng_tgt main.cc)
target_link_libraries(tstgslrng_tgt GSL::gsl)
add_test(NAME tstgslrng_tgt COMMAND tstgslrng_tgt)

add_executable(tstgslrng_var main.cc)
target_link_libraries(tstgslrng_var ${GSL_LIBRARIES})
target_include_directories(tstgslrng_var PRIVATE ${GSL_INCLUDE_DIRS})
add_test(NAME tstgslrng_var COMMAND tstgslrng_var)
