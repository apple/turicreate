
enable_language(CXX)

add_library(foo SHARED IMPORTED)

add_library(alias ALIAS foo)
