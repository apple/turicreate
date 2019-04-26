
add_library(piciface UNKNOWN IMPORTED)
set_property(TARGET piciface PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)

add_executable(conflict "main.cpp")
set_property(TARGET conflict PROPERTY POSITION_INDEPENDENT_CODE OFF)
target_link_libraries(conflict piciface)
