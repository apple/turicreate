
enable_language(CXX)

file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<BOOL:$<TARGET_PROPERTY:foo,SOURCES>>somefile.cpp"
  CONTENT "static const char content[] = \"$<TARGET_PROPERTY:foo,SOURCES>\";\n"
)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/renamed.cpp"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp" "${CMAKE_CURRENT_BINARY_DIR}/renamed.cpp")

add_executable(foo "${CMAKE_CURRENT_BINARY_DIR}/1somefile.cpp" "${CMAKE_CURRENT_BINARY_DIR}/renamed.cpp")
