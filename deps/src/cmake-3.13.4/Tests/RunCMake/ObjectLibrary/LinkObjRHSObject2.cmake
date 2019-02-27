cmake_policy(SET CMP0022 NEW)

enable_language(C)

add_library(AnObjLib OBJECT a.c)
target_compile_definitions(AnObjLib INTERFACE REQUIRED)

add_library(AnotherObjLib OBJECT b.c)
target_link_libraries(AnotherObjLib PUBLIC AnObjLib)

add_executable(exe exe.c)
target_link_libraries(exe AnotherObjLib)
