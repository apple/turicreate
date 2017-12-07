
cmake_minimum_required(VERSION 2.8.4)

cmake_policy(SET CMP0004 NEW)

add_library(foo SHARED empty.cpp)
add_library(bar SHARED empty.cpp)

target_link_libraries(foo "$<1: bar >")
