cmake_policy(VERSION 3.2)
cmake_policy(SET CMP0062 NEW)

add_library(iface INTERFACE)
export(TARGETS iface FILE "${CMAKE_CURRENT_BINARY_DIR}/exported.cmake")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/exported.cmake" DESTINATION cmake)
