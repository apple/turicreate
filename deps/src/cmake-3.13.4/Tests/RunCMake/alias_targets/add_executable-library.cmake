
enable_language(CXX)

add_library(foo empty.cpp)

add_executable(alias ALIAS foo)
