enable_language(C)

add_executable(top empty.c)
add_subdirectory(CMP0079-iface)
get_property(libs TARGET top PROPERTY INTERFACE_LINK_LIBRARIES)
message(STATUS "INTERFACE_LINK_LIBRARIES='${libs}'")
