set(CMAKE_CONFIGURATION_TYPES "Release" CACHE INTERNAL "Supported configuration types")

project(XcodePreserveNonOptimizationFlags CXX)

add_library(preserveStart STATIC foo.cpp)
set_property(TARGET preserveStart PROPERTY COMPILE_OPTIONS -DA -O1)

add_library(preserveBoth STATIC foo.cpp)
set_property(TARGET preserveBoth PROPERTY COMPILE_OPTIONS -DB -O1 -DC)

add_library(preserveEnd STATIC foo.cpp)
set_property(TARGET preserveEnd PROPERTY COMPILE_OPTIONS -O1 -DD)
