enable_language(C)
set(CMAKE_C_CLANG_TIDY "${PSEUDO_TIDY}" -some -args)
add_executable(main main.c)
