project(LinkObjLHSStatic C)

add_library(OtherLib STATIC a.c)
target_compile_definitions(OtherLib INTERFACE REQUIRED)

add_library(AnObjLib OBJECT requires.c)
target_link_libraries(AnObjLib OtherLib)
