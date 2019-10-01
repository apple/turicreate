
add_library(foo UNKNOWN IMPORTED)

set_property(TARGET foo APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SOMETHING)
set_property(TARGET foo APPEND PROPERTY COMPATIBLE_INTERFACE_STRING SOMETHING)

add_executable(user main.cpp)
target_link_libraries(user foo)
