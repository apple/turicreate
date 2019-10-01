add_library(bar STATIC bar.cpp)

add_library(foo STATIC foo.cpp)
target_include_directories(foo PUBLIC .)
