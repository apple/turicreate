
enable_language(CXX)

add_library(foo empty.cpp)
add_library(bar empty.cpp)

add_library(alias ALIAS foo)

target_link_libraries(alias bar)
