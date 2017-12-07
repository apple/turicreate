cmake_minimum_required(VERSION 2.8)
project(LinkDirectoryExternal C)

# Test CMP0015 OLD behavior: -L../lib
cmake_policy(SET CMP0015 OLD)
link_directories(../lib)

# Test CMP0015 NEW behavior: -L${CMAKE_CURRENT_SOURCE_DIR}/lib
cmake_policy(SET CMP0015 NEW)
link_directories(lib)

add_executable(myexe myexe.c)
set_property(TARGET myexe PROPERTY OUTPUT_NAME LinkDirectory)
target_link_libraries(myexe mylibA mylibB)
