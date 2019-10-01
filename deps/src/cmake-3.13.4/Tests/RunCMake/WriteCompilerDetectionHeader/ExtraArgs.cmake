
include(WriteCompilerDetectionHeader)

write_compiler_detection_header(
  FILE "${CMAKE_CURRENT_BINARY_DIR}/somefile"
  PREFIX Pref
  GarbageArg
  COMPILERS GNU
  FEATURES cxx_final
)
