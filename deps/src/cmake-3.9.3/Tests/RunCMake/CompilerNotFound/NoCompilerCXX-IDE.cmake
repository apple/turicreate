set(CMAKE_CXX_COMPILER_ID_ERROR_FOR_TEST "#error NoCompilerCXX-IDE")
enable_language(CXX)
message(FATAL_ERROR "This error should not be reached.")
