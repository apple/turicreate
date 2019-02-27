cmake_policy(SET CMP0069 NEW)

set(_CMAKE_CXX_IPO_SUPPORTED_BY_CMAKE NO)

add_executable(foo main.cpp)
set_target_properties(foo PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
