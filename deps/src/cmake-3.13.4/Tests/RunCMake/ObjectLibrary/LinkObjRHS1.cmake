add_library(A STATIC a.c)
add_library(AnObjLib OBJECT a.c)
target_link_libraries(A AnObjLib)
