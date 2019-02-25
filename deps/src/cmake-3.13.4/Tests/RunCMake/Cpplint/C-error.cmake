enable_language(C)
set(CMAKE_C_CPPLINT "${PSEUDO_CPPLINT}" --error)
add_executable(main main.c)
