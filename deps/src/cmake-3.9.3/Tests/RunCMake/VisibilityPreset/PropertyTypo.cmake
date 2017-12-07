enable_language(CXX)

# Ensure CMake warns even if toolchain does not really have these flags.
set(CMAKE_CXX_COMPILE_OPTIONS_VISIBILITY_INLINES_HIDDEN "-fvisibility-inlines-hidden")
set(CMAKE_CXX_COMPILE_OPTIONS_VISIBILITY "-fvisibility=")

add_library(visibility_preset SHARED lib.cpp)
set_property(TARGET visibility_preset PROPERTY CXX_VISIBILITY_PRESET hiden)
