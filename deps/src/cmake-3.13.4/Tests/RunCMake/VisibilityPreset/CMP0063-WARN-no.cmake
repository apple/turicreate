
enable_language(CXX)

# Ensure CMake does not warn even if toolchain really does have these flags.
unset(CMAKE_CXX_COMPILE_OPTIONS_VISIBILITY_INLINES_HIDDEN)
unset(CMAKE_CXX_COMPILE_OPTIONS_VISIBILITY)

include(CMP0063-Common.cmake)
