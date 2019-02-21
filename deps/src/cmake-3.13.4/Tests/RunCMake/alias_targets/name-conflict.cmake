
enable_language(CXX)

add_library(foo empty.cpp)

add_library(bar ALIAS foo)

add_library(bar empty.cpp)
