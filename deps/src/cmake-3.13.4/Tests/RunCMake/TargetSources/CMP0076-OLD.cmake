cmake_policy(SET CMP0076 OLD)

add_library(iface INTERFACE)
target_sources(iface INTERFACE empty_1.cpp)

get_property(iface_sources TARGET iface PROPERTY INTERFACE_SOURCES)
message(STATUS "iface: ${iface_sources}")

add_executable(main main.cpp)
target_link_libraries(main iface)
