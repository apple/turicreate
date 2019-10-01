
project(CMP0022-WARN-tll)

add_library(foo SHARED empty_vs6_1.cpp)
add_library(bar SHARED empty_vs6_2.cpp)
add_library(bat SHARED empty_vs6_3.cpp)
target_link_libraries(bar LINK_PUBLIC foo)
# Replace the compatibility values set by target_link_libraries
set_property(TARGET bar PROPERTY LINK_INTERFACE_LIBRARIES bat)
set_property(TARGET bar PROPERTY LINK_INTERFACE_LIBRARIES_DEBUG bat)

add_library(user SHARED empty.cpp)
target_link_libraries(user bar)
