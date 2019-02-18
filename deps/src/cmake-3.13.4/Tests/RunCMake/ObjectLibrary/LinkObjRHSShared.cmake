enable_language(C)

add_definitions(-DCOMPILE_FOR_SHARED_LIB)

add_library(AnObjLib OBJECT a.c)
target_compile_definitions(AnObjLib INTERFACE REQUIRED)
set_target_properties(AnObjLib PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(A SHARED b.c $<TARGET_OBJECTS:AnObjLib>)
target_link_libraries(A PUBLIC AnObjLib)

add_executable(exe exe.c)
target_link_libraries(exe A)
