
enable_language(C)

add_library(empty empty.c)
target_sources(empty PRIVATE empty.$<COMPILE_LANGUAGE>)
