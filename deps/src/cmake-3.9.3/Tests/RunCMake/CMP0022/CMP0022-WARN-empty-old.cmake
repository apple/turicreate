
project(CMP0022-WARN-empty-old)

add_library(foo SHARED empty_vs6_1.cpp)
add_library(bar SHARED empty_vs6_2.cpp)

set_property(TARGET bar PROPERTY INTERFACE_LINK_LIBRARIES foo)

add_library(user empty.cpp)
target_link_libraries(user bar)
