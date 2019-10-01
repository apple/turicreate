
include(WriteCompilerDetectionHeader)

write_compiler_detection_header(
  FILE "${CMAKE_CURRENT_BINARY_DIR}/somefile"
  PREFIX Pref
  OUTPUT_FILES_VAR outfiles
  COMPILERS GNU
  VERSION 3.1
  FEATURES cxx_auto_type
)
