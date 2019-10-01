
add_library(picon INTERFACE)
set_property(TARGET picon PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)

add_library(picoff INTERFACE)
set_property(TARGET picoff PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE OFF)

add_executable(conflict "main.cpp")
target_link_libraries(conflict picon picoff)
