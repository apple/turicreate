
project(cmp0022NEW)

cmake_policy(SET CMP0022 NEW)

add_library(cmp0022NEW SHARED empty_vs6_1.cpp)
add_library(testLib SHARED empty_vs6_2.cpp)

set_property(TARGET cmp0022NEW APPEND PROPERTY LINK_INTERFACE_LIBRARIES testLib)

export(TARGETS cmp0022NEW testLib FILE "${CMAKE_CURRENT_BINARY_DIR}/cmp0022NEW.cmake")
