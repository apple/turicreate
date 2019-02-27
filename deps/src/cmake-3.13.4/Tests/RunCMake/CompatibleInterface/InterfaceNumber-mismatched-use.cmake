
add_library(foo UNKNOWN IMPORTED)
add_library(bar UNKNOWN IMPORTED)

set_property(TARGET foo APPEND PROPERTY COMPATIBLE_INTERFACE_NUMBER_MIN SOMEPROP)
set_property(TARGET foo PROPERTY INTERFACE_SOMEPROP 42)

add_executable(user main.cpp)
target_link_libraries(user foo $<$<STREQUAL:$<TARGET_PROPERTY:SOMEPROP>,42>:bar>)
