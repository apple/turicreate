project(LinkObjLHSShared C)

add_library(OtherLib SHARED a.c)
target_compile_definitions(OtherLib INTERFACE REQUIRED)

add_library(AnObjLib OBJECT requires.c)
target_link_libraries(AnObjLib OtherLib)
