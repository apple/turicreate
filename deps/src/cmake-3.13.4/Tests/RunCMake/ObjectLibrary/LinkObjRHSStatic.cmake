enable_language(C)

add_library(AnObjLib OBJECT a.c)
target_compile_definitions(AnObjLib INTERFACE REQUIRED)

add_library(A STATIC b.c $<TARGET_OBJECTS:AnObjLib>)
target_link_libraries(A PUBLIC AnObjLib)

add_executable(exe exe.c)
target_link_libraries(exe A)
