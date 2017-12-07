
project(CMP0022-WARN)

add_library(foo STATIC empty_vs6_1.cpp)
add_library(bar STATIC empty_vs6_2.cpp)
add_library(bat STATIC empty_vs6_3.cpp)
set_property(TARGET bar PROPERTY INTERFACE_LINK_LIBRARIES foo)
set_property(TARGET bar PROPERTY LINK_LIBRARIES bat)

add_library(user empty.cpp)
target_link_libraries(user bar)
