set(CMAKE_C_COMPILER_ID_ERROR_FOR_TEST "#error NoCompilerC-IDE")
enable_language(C)
message(FATAL_ERROR "This error should not be reached.")
