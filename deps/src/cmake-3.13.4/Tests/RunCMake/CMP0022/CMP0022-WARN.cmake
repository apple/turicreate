
project(CMP0022-WARN)

cmake_policy(SET CMP0042 NEW)

add_library(foo SHARED empty_vs6_1.cpp)
add_library(bar SHARED empty_vs6_2.cpp)
add_library(bat SHARED empty_vs6_3.cpp)
set_property(TARGET bar PROPERTY INTERFACE_LINK_LIBRARIES foo)
set_property(TARGET bar PROPERTY LINK_INTERFACE_LIBRARIES bat)

add_library(user empty.cpp)
target_link_libraries(user bar)

# Use "bar" again with a different "head" target to check
# that the warning does not appear again.
add_library(user2 empty_vs6_3.cpp)
target_link_libraries(user2 bar)
