cmake_minimum_required(VERSION 2.7.20090708)
project(LinkLanguage C CXX)

add_library(foo STATIC foo.cxx)
add_executable(LinkLanguage LinkLanguage.c)

# Export the target now to compute its link interface and implementation.
# This tests that the link info is recomputed after the library is linked.
export(TARGETS LinkLanguage FILE LinkLanguageTargets.cmake)

target_link_libraries(LinkLanguage foo)

# CMake should now automatically choose CXX for linking, so we need
# not set the property:
#set_property(TARGET LinkLanguage PROPERTY LINKER_LANGUAGE CXX)
