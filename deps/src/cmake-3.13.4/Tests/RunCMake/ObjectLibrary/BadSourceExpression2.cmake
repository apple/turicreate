add_library(A STATIC a.c $<TARGET_OBJECTS:DoesNotExist>)
