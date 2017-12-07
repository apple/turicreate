
enable_language(CXX)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/copied_file.cpp"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp" "${CMAKE_CURRENT_BINARY_DIR}/copied_file$<TARGET_PROPERTY:NotAProperty>.cpp"
)

add_library(empty "${CMAKE_CURRENT_BINARY_DIR}/copied_file.cpp")
