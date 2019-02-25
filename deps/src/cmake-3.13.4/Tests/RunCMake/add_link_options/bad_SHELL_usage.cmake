
enable_language(C)

add_link_options("LINKER:-foo,SHELL:-bar")

add_library(example SHARED LinkOptionsLib.c)
