set(CMAKE_C_COMPILER_ID_ERROR_FOR_TEST "#error NoCompilerCandCXX-IDE")
set(CMAKE_CXX_COMPILER_ID_ERROR_FOR_TEST "#error NoCompilerCandCXX-IDE")
project(NoCompilerCandCXXInner C CXX)
message(FATAL_ERROR "This error should not be reached.")
