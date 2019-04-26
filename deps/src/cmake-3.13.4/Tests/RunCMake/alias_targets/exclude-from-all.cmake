
enable_language(CXX)

add_library(foo empty.cpp)

add_library(alias ALIAS EXCLUDE_FROM_ALL foo)
