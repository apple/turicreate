
cmake_policy(SET CMP0028 NEW)

add_library(iface INTERFACE)
target_link_libraries(iface INTERFACE External::Library)
add_library(foo empty.cpp)
target_link_libraries(foo iface)
