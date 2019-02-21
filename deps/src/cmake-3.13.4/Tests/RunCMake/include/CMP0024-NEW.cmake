
enable_language(CXX)

cmake_policy(SET CMP0024 NEW)

add_library(foo SHARED empty.cpp)

add_subdirectory(subdir1)
add_subdirectory(subdir2)
