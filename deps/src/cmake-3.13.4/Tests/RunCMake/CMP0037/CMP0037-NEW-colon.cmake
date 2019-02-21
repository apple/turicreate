enable_language(CXX)
cmake_policy(SET CMP0037 NEW)

add_library("lib:colon" empty.cpp)
add_executable("exe:colon" empty.cpp)
add_custom_target("custom:colon")
