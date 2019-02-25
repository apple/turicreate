add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/copied_file.cpp"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp" "${CMAKE_CURRENT_BINARY_DIR}/copied_file$<TARGET_POLICY:CMP0004>.cpp"
)
add_custom_target(drive DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/copied_file.cpp")
