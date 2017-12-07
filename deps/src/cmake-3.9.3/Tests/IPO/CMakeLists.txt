cmake_minimum_required (VERSION 2.8)
project(IPO NONE)

set_property(DIRECTORY PROPERTY INTERPROCEDURAL_OPTIMIZATION 1)

add_subdirectory(../COnly COnly)
add_subdirectory(../CxxOnly CxxOnly)
