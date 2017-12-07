add_library(A OBJECT a.c)
target_sources(A PRIVATE $<TARGET_OBJECTS:A>)
