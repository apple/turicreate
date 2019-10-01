
enable_language(CXX)

add_library(foo empty.cpp)

add_library(alias ALIAS foo)

set_target_properties(alias PROPERTIES ANYTHING 1)
