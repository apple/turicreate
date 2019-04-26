enable_language(C)

add_library(AnObjLib OBJECT a.c)
target_compile_definitions(AnObjLib INTERFACE REQUIRED)

add_library(A STATIC b.c)
target_link_libraries(A PRIVATE AnObjLib)
target_compile_definitions(A INTERFACE $<TARGET_PROPERTY:AnObjLib,INTERFACE_COMPILE_DEFINITIONS>)

add_executable(exe exe.c)
target_link_libraries(exe A)
