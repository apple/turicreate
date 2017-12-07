
project(CMP0022-WARN)

add_library(foo SHARED empty_vs6_1.cpp)
add_library(bar SHARED empty_vs6_2.cpp)
add_library(bat SHARED empty_vs6_3.cpp)

target_link_libraries(foo bar)
target_link_libraries(foo PRIVATE bat)
