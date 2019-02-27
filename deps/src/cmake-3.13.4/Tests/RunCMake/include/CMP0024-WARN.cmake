
enable_language(CXX)

add_library(foo SHARED empty.cpp)

add_subdirectory(subdir1)
add_subdirectory(subdir2)
