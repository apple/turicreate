
cmake_policy(SET CMP0041 OLD)

add_library(foo empty.cpp)
set_property(TARGET foo
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    include/$<TARGET_PROPERTY:NAME>
    ${CMAKE_CURRENT_SOURCE_DIR}/include/$<TARGET_PROPERTY:NAME>
    ${CMAKE_CURRENT_BINARY_DIR}/include/$<TARGET_PROPERTY:NAME>
)
install(TARGETS foo EXPORT FooExport DESTINATION lib)
install(EXPORT FooExport DESTINATION lib/cmake)
