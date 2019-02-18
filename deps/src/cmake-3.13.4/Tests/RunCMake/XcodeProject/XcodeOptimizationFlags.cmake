set(CMAKE_CONFIGURATION_TYPES "Release" CACHE INTERNAL "Supported configuration types")

set(CMAKE_CXX_FLAGS_RELEASE "")

project(XcodeOptimizationFlags CXX)

add_library(fooO1 STATIC foo.cpp)
set_target_properties(fooO1 PROPERTIES COMPILE_OPTIONS -O1)

add_library(fooO2 STATIC foo.cpp)
set_target_properties(fooO2 PROPERTIES COMPILE_OPTIONS -O2)

add_library(fooO3 STATIC foo.cpp)
set_target_properties(fooO3 PROPERTIES COMPILE_OPTIONS -O3)

add_library(fooOs STATIC foo.cpp)
set_target_properties(fooOs PROPERTIES COMPILE_OPTIONS -Os)

add_library(fooOfast STATIC foo.cpp)
set_target_properties(fooOfast PROPERTIES COMPILE_OPTIONS -Ofast)
