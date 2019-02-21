cmake_minimum_required(VERSION 2.8)
project(LinkDirectoryExternal C)


add_executable(myexe2 myexe.c)
set_property(TARGET myexe2 PROPERTY OUTPUT_NAME LinkDirectory2)
target_link_directories(myexe2 PRIVATE lib "${CMAKE_CURRENT_SOURCE_DIR}/../lib")
target_link_libraries(myexe2 PRIVATE mylibA mylibB)

add_library (mylibs INTERFACE)
target_link_directories(mylibs INTERFACE lib "${CMAKE_CURRENT_SOURCE_DIR}/../lib")
target_link_libraries(mylibs INTERFACE mylibA mylibB)
add_executable(myexe3 myexe.c)
set_property(TARGET myexe3 PROPERTY OUTPUT_NAME LinkDirectory3)
target_link_libraries(myexe3 PRIVATE mylibs)


# Test CMP0015 OLD behavior: -L../lib
cmake_policy(SET CMP0015 OLD)
link_directories(../lib)

# Test CMP0015 NEW behavior: -L${CMAKE_CURRENT_SOURCE_DIR}/lib
cmake_policy(SET CMP0015 NEW)
link_directories(lib)

add_executable(myexe myexe.c)
set_property(TARGET myexe PROPERTY OUTPUT_NAME LinkDirectory)
target_link_libraries(myexe mylibA mylibB)
