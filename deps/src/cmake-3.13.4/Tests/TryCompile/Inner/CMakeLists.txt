cmake_minimum_required(VERSION 2.6)
project(TryCompileInner C)

try_compile(SHOULD_PASS
    ${TryCompileInner_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp
    ${TryCompileInner_SOURCE_DIR}/../pass.c
    OUTPUT_VARIABLE TRY_OUT
    )
if(NOT SHOULD_PASS)
  message(FATAL_ERROR "Inner try-compile SHOULD_PASS failed!")
endif()

add_library(innerlib innerlib.c)
add_executable(innerexe innerexe.c)
target_link_libraries(innerexe innerlib)
