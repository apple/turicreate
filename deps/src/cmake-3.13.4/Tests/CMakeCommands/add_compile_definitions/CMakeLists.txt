cmake_minimum_required(VERSION 3.1)

project(add_compile_definitions LANGUAGES CXX)

add_compile_definitions(TEST_DEFINITION
                        $<$<COMPILE_LANGUAGE:CXX>:LANG_$<COMPILE_LANGUAGE>>
                        $<$<EQUAL:0,1>:UNEXPECTED_DEFINITION>)

add_executable(add_compile_definitions main.cpp)

add_library(imp UNKNOWN IMPORTED)
get_target_property(_res imp COMPILE_DEFINITIONS)
if (_res)
  message(SEND_ERROR "add_compile_definitions populated the COMPILE_DEFINITIONS target property")
endif()
