
include(WriteCompilerDetectionHeader)

write_compiler_detection_header(
  FILE "${CMAKE_CURRENT_BINARY_DIR}/somefile"
  PREFIX "0compile"
  VERSION 3.1
  COMPILERS GNU
  FEATURES cxx_final
)
