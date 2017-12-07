cmake_minimum_required (VERSION 2.6)
project(SameName C)

add_subdirectory(Lib1)

include_directories(${PROJECT_SOURCE_DIR}/Lib1)
add_subdirectory(Exe1)
