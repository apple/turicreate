enable_language(C)

add_definitions(-DCOMPILE_FOR_SHARED_LIB)

add_library(AnObjLib OBJECT a.c)
target_compile_definitions(AnObjLib INTERFACE REQUIRED)
set_target_properties(AnObjLib PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(A SHARED b.c)
target_link_libraries(A PRIVATE AnObjLib)
target_compile_definitions(A INTERFACE $<TARGET_PROPERTY:AnObjLib,INTERFACE_COMPILE_DEFINITIONS>)

add_executable(exe exe.c)
target_link_libraries(exe A)
