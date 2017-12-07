cmake_policy(SET CMP0069 OLD)

add_executable(foo main.cpp)
set_target_properties(foo PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
