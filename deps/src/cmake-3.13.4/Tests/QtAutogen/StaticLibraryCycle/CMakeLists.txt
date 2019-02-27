cmake_minimum_required(VERSION 3.10)
project(StaticLibraryCycle)
include("../AutogenTest.cmake")

# Test AUTOMOC on cyclic static libraries

set(CMAKE_AUTOMOC ON)

# Cyclic static libraries
add_library(slc_a STATIC a.cpp)
target_link_libraries(slc_a ${QT_LIBRARIES} slc_b)

add_library(slc_b STATIC b.cpp)
target_link_libraries(slc_b ${QT_LIBRARIES} slc_c)

add_library(slc_c STATIC c.cpp)
target_link_libraries(slc_c ${QT_LIBRARIES} slc_a)

add_executable(slc main.cpp)
target_link_libraries(slc ${QT_LIBRARIES} slc_a)
