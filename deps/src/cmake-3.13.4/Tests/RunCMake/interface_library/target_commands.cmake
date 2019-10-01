
add_library(iface INTERFACE)

target_link_libraries(iface PRIVATE foo)
target_link_libraries(iface PUBLIC foo)
target_link_libraries(iface foo)
target_link_libraries(iface LINK_INTERFACE_LIBRARIES foo)

target_include_directories(iface PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(iface PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(iface PRIVATE SOME_DEFINE)
target_compile_definitions(iface PUBLIC SOME_DEFINE)
