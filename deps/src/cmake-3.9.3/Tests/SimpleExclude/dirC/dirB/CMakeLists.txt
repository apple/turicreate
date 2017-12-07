add_library(t6 STATIC t6.c)

add_library(t7 STATIC t7.c)
target_link_libraries(t7 t2)

