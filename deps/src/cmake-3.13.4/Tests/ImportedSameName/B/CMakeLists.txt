add_library(b STATIC b.c)
target_compile_definitions(b INTERFACE DEF_B)

add_library(sameName INTERFACE IMPORTED)
target_link_libraries(sameName INTERFACE b)

add_library(ifaceB INTERFACE)
target_link_libraries(ifaceB INTERFACE sameName)
