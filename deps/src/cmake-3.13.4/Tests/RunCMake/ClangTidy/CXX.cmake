enable_language(CXX)
set(CMAKE_CXX_CLANG_TIDY "${PSEUDO_TIDY}" -some -args)
add_executable(main main.cxx)
