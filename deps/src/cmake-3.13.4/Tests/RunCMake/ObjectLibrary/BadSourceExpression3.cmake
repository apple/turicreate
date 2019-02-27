add_library(NotObjLib STATIC a.c)
add_library(A STATIC a.c $<TARGET_OBJECTS:NotObjLib>)
