cmake_minimum_required(VERSION 3.0)
project(FindOpenSSL_rand CXX)
include(CTest)

find_package(OpenSSL REQUIRED)

add_executable(tstopensslrand_tgt main.cc)
target_link_libraries(tstopensslrand_tgt OpenSSL::SSL)
add_test(NAME tstopensslrand_tgt COMMAND tstopensslrand_tgt)

add_executable(tstopensslrand_var main.cc)
target_link_libraries(tstopensslrand_var ${OPENSSL_LIBRARIES})
target_include_directories(tstopensslrand_var PRIVATE ${OPENSSL_INCLUDE_DIR})
add_test(NAME tstopensslrand_var COMMAND tstopensslrand_var)
