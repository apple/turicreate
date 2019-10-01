enable_language(CXX)
cmake_policy(SET CMP0037 OLD)

add_library(all empty.cpp)
add_executable(clean empty.cpp)
add_custom_target(help)
