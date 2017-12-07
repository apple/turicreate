cmake_minimum_required(VERSION 3.3)

project(IOSInstallCombined CXX)

set(CMAKE_OSX_SYSROOT iphoneos)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf")
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")

set(CMAKE_OSX_ARCHITECTURES "armv7;arm64;i386;x86_64")

add_executable(foo_app MACOSX_BUNDLE main.cpp)
install(TARGETS foo_app BUNDLE DESTINATION bin)

add_library(foo_static STATIC foo.cpp)
install(TARGETS foo_static ARCHIVE DESTINATION lib)

add_library(foo_shared SHARED foo.cpp)
install(TARGETS foo_shared LIBRARY DESTINATION lib)

add_library(foo_bundle MODULE foo.cpp)
set_target_properties(foo_bundle PROPERTIES BUNDLE TRUE)
install(TARGETS foo_bundle LIBRARY DESTINATION lib)

add_library(foo_framework SHARED foo.cpp)
set_target_properties(foo_framework PROPERTIES FRAMEWORK TRUE)
install(TARGETS foo_framework FRAMEWORK DESTINATION lib)
