enable_language(CXX)
add_library(foo empty.cpp)
export(TARGETS foo FILE "${CMAKE_CURRENT_BINARY_DIR}/foo.cmake")
install(TARGETS foo EXPORT fooExport
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
export(EXPORT fooExport
  EXPORT_LINK_INTERFACE_LIBRARIES
)
