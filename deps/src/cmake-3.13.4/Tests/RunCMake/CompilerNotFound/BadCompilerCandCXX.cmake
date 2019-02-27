set(CMAKE_C_COMPILER "no-C-compiler")
set(CMAKE_CXX_COMPILER "no-CXX-compiler")
project(BadCompilerCandCXXInner C CXX)
message(FATAL_ERROR "This error should not be reached.")
