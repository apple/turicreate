
enable_language(CXX)

add_library(foo empty.cpp)

add_library(alias ALIAS foo)

set_property(TARGET alias PROPERTY ANYTHING 1)
