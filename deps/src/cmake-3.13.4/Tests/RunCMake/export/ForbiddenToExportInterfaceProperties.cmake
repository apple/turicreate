enable_language(CXX)
add_library(foo empty.cpp)
set_target_properties(foo PROPERTIES
  INTERFACE_FOOBAR "Some string"
  EXPORT_PROPERTIES "INTERFACE_FOOBAR"
)
export(TARGETS foo FILE "${CMAKE_CURRENT_BINARY_DIR}/foo.cmake")
install(TARGETS foo EXPORT fooExport
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
