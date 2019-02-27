
include(WriteCompilerDetectionHeader)

write_compiler_detection_header(
    FILE "${CMAKE_CURRENT_BINARY_DIR}/somefile"
    PREFIX PREF_
    COMPILERS GNU
    # FEATURES
    VERSION 3.1
)
