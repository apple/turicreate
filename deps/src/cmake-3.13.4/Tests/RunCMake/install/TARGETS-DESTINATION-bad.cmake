enable_language(C)
add_library(empty empty.c)
install(TARGETS empty DESTINATION $<NOTAGENEX>)
