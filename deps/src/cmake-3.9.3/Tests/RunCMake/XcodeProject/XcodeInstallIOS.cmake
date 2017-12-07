cmake_minimum_required(VERSION 2.8.5)

project(XcodeInstallIOS)

set(CMAKE_OSX_SYSROOT iphoneos)
set(XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")

set(CMAKE_OSX_ARCHITECTURES "armv7;i386")

add_library(foo STATIC foo.cpp)
install(TARGETS foo ARCHIVE DESTINATION lib)
