enable_language(C)
set(CMAKE_C_INCLUDE_WHAT_YOU_USE "${PSEUDO_IWYU}" -some -args)
add_executable(main main.c)
