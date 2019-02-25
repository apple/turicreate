
enable_language(CXX)

add_subdirectory(dir1)

# This affects dir1 despite being set after the add_subdirectory.
cmake_policy(SET CMP0044 NEW)
