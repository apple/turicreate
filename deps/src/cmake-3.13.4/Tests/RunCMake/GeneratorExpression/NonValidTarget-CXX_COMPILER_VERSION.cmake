add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/copied_file.cpp"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp" "${CMAKE_CURRENT_BINARY_DIR}/copied_file$<CXX_COMPILER_VERSION>.cpp"
)
add_custom_target(drive DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/copied_file.cpp")
