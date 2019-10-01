add_library(A SHARED a.c)
target_link_libraries(A AnObjLib)
add_library(AnObjLib OBJECT a.c)
