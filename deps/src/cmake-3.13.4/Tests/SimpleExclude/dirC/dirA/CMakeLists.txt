add_library(t1 STATIC t1.c)

add_library(t2 STATIC t2.c)

add_executable(t3 t3.c)

add_executable(t4 t4.c)

add_executable(t5 t5.c)
target_link_libraries(t5 t1)
