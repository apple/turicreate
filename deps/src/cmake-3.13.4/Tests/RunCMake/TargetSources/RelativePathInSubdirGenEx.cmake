cmake_policy(SET CMP0076 NEW)

add_library(genexlib)
add_subdirectory(RelativePathInSubdirGenEx)

get_property(genexlib_sources TARGET genexlib PROPERTY SOURCES)
message(STATUS "genexlib: ${genexlib_sources}")

add_executable(genexmain main.cpp)
target_link_libraries(genexmain genexlib)
