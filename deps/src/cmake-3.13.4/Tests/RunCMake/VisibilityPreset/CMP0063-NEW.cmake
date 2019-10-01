cmake_policy(SET CMP0063 NEW)
enable_language(CXX)

# Ensure CMake would warn even if toolchain does not really have these flags.
set(CMAKE_CXX_COMPILE_OPTIONS_VISIBILITY_INLINES_HIDDEN "-fvisibility-inlines-hidden")
set(CMAKE_CXX_COMPILE_OPTIONS_VISIBILITY "-fvisibility=")

include(CMP0063-Common.cmake)
