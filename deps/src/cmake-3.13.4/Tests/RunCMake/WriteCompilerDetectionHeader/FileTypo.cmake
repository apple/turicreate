
include(WriteCompilerDetectionHeader)

write_compiler_detection_header(
  FILE_TYPO "${CMAKE_CURRENT_BINARY_DIR}/somefile"
  PREFIX Pref
)
