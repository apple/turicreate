add_library(imp::iface INTERFACE IMPORTED)
set_property(TARGET imp::iface APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SOMEPROP)
set_property(TARGET imp::iface PROPERTY INTERFACE_SOMEPROP ON)
set_property(TARGET imp::iface PROPERTY INTERFACE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sub.cpp)

add_library(subiface INTERFACE)
target_link_libraries(subiface INTERFACE imp::iface)
set_property(TARGET subiface PROPERTY INTERFACE_SOMEPROP ON)
