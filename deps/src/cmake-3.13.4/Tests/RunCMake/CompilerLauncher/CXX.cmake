enable_language(CXX)
set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_COMMAND};-E;env;USED_LAUNCHER=1")
set(CMAKE_VERBOSE_MAKEFILE TRUE)
add_executable(main main.cxx)
