enable_language(C)
set(CMAKE_C_CLANG_TIDY "${PSEUDO_TIDY}" -bad)
add_executable(main main.c)
