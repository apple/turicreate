
enable_language(CXX)

add_library(foo empty_vs6_1.cpp)
add_library(bar empty_vs6_2.cpp)
add_library(bat empty_vs6_3.cpp)

target_link_libraries(foo LINK_PRIVATE bar)
target_link_libraries(foo PRIVATE bat)
