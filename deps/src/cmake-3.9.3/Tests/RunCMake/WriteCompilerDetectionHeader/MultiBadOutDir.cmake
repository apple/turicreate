
include(WriteCompilerDetectionHeader)

write_compiler_detection_header(
  FILE "${CMAKE_CURRENT_BINARY_DIR}/dir/somefile"
  PREFIX Pref
  OUTPUT_FILES_VAR outfiles
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}"
  COMPILERS GNU
  VERSION 3.1
  FEATURES cxx_auto_type
)
