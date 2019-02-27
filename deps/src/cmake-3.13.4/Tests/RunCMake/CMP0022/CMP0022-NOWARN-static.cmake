
project(CMP0022-NOWARN-static)

add_library(foo STATIC empty_vs6_1.cpp)
add_library(bar STATIC empty_vs6_2.cpp)
add_library(bat STATIC empty_vs6_3.cpp)
target_link_libraries(foo bar)
target_link_libraries(bar bat)
