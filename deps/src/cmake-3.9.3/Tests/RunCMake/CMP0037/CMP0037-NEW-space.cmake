
cmake_policy(SET CMP0037 NEW)

add_library("lib with spaces" empty.cpp)
add_executable("exe with spaces" empty.cpp)
add_custom_target("custom with spaces")
