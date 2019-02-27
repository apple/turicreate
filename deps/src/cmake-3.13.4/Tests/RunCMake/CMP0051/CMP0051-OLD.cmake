
cmake_policy(SET CMP0051 OLD)

add_library(objects OBJECT empty.cpp)

add_library(empty empty.cpp $<TARGET_OBJECTS:objects>)

get_target_property(srcs empty SOURCES)

message("Sources: \"${srcs}\"")
