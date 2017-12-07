
cmake_policy(SET CMP0041 NEW)

add_library(foo empty.cpp)
target_include_directories(foo INTERFACE
  include/$<TARGET_PROPERTY:NAME>
  ${CMAKE_CURRENT_SOURCE_DIR}/include/$<TARGET_PROPERTY:NAME>
  ${CMAKE_CURRENT_BINARY_DIR}/include/$<TARGET_PROPERTY:NAME>
)
install(TARGETS foo EXPORT FooExport DESTINATION lib)
install(EXPORT FooExport DESTINATION lib/cmake)
