
enable_language(CXX)

add_library(foo empty.cpp)

add_library(invalid$name ALIAS foo)
