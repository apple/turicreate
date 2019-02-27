
add_library(foo UNKNOWN IMPORTED)
add_library(bar UNKNOWN IMPORTED)

set_property(TARGET foo APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SOMEPROP)
set_property(TARGET foo PROPERTY INTERFACE_SOMEPROP ON)
set_property(TARGET bar PROPERTY INTERFACE_SOMEPROP OFF)

add_executable(user main.cpp)
target_link_libraries(user foo bar)
