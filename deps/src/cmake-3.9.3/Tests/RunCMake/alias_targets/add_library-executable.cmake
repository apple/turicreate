
enable_language(CXX)

add_executable(foo empty.cpp)

add_library(alias ALIAS foo)
