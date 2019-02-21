enable_language(C)
set(CMAKE_C_CPPCHECK "${PSEUDO_CPPCHECK}" -bad)
add_executable(main main.c)
