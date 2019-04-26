
include(WriteCompilerDetectionHeader)

write_compiler_detection_header(
  FILE "${CMAKE_CURRENT_BINARY_DIR}/somefile"
  PREFIX Pref
  VERSION 3.0
  COMPILERS GNU
  FEATURES cxx_final
)
