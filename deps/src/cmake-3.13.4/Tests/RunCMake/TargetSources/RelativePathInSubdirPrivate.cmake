cmake_policy(SET CMP0076 NEW)

add_library(privatelib)

add_subdirectory(RelativePathInSubdirPrivate)

get_property(privatelib_sources TARGET privatelib PROPERTY SOURCES)
message(STATUS "privatelib: ${privatelib_sources}")
