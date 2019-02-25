enable_language(CXX)
add_library(foo empty.cpp)
set_target_properties(foo PROPERTIES
  JUST_A_PROPERTY "$<C_COMPILER_VERSION:0>"
  EXPORT_PROPERTIES "JUST_A_PROPERTY"
)
export(TARGETS foo FILE "${CMAKE_CURRENT_BINARY_DIR}/foo.cmake")
install(TARGETS foo EXPORT fooExport
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
