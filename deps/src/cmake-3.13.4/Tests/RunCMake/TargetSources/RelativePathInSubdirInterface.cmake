cmake_policy(SET CMP0076 NEW)

add_library(iface INTERFACE)

add_subdirectory(RelativePathInSubdirInterface)

get_property(iface_sources TARGET iface PROPERTY INTERFACE_SOURCES)
message(STATUS "iface: ${iface_sources}")

add_executable(main main.cpp)
target_link_libraries(main iface)
