
add_library(objects OBJECT empty.cpp)

add_library(empty empty.cpp $<TARGET_OBJECTS:objects>)

get_target_property(srcs empty SOURCES)

message("Sources: \"${srcs}\"")

add_subdirectory(CMP0051-WARN-Dir)

get_target_property(srcs empty2 SOURCES)

message("Sources: \"${srcs}\"")
