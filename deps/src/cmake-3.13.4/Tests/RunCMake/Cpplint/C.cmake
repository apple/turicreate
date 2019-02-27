enable_language(C)
set(CMAKE_C_CPPLINT "${PSEUDO_CPPLINT}" --verbose=0 --linelength=80)
add_executable(main main.c)
