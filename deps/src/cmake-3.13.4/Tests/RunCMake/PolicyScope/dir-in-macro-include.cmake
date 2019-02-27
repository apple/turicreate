
enable_language(CXX)

# This does not affect dir1 despite being set before the add_subdirectory.
cmake_policy(SET CMP0044 NEW)
add_subdirectory(dir1)
