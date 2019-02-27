
enable_language(CXX)

add_library(foo empty.cpp)

add_library(alias ALIAS foo)

add_library(next_alias ALIAS alias)
