add_library(t8 STATIC t8.c)

add_executable(t9 t9.c)
target_link_libraries(t9 t7)

add_custom_target(t4_custom ALL)
add_dependencies(t4_custom t4)
