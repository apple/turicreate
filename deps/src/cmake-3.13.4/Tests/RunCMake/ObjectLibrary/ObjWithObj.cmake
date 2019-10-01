add_library(A OBJECT a.c)
add_library(B OBJECT $<TARGET_OBJECTS:A>)
