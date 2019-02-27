add_library(a STATIC a.c)
target_compile_definitions(a INTERFACE DEF_A)

add_library(sameName INTERFACE IMPORTED)
target_link_libraries(sameName INTERFACE a)

add_library(ifaceA INTERFACE)
target_link_libraries(ifaceA INTERFACE sameName)
