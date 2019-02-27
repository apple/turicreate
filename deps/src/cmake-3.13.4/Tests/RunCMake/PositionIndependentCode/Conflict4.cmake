
add_library(piciface INTERFACE)

add_library(picon INTERFACE)
set_property(TARGET picon PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)

add_executable(conflict "main.cpp")
target_link_libraries(conflict picon $<$<BOOL:$<TARGET_PROPERTY:POSITION_INDEPENDENT_CODE>>:piciface>)
