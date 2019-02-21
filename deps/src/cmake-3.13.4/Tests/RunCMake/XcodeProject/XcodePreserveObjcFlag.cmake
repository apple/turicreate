set(CMAKE_CONFIGURATION_TYPES "Release" CACHE INTERNAL "Supported configuration types")

project(XcodePreserveObjcFlag CXX)

add_library(foo STATIC foo.cpp)
set_target_properties(foo PROPERTIES COMPILE_OPTIONS -ObjC)
