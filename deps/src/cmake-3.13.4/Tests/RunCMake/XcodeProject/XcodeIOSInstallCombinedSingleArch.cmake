cmake_minimum_required(VERSION 3.3)

project(XcodeIOSInstallCombinedSingleArch CXX)

# due to lack of toolchain file it might point to running macOS version
unset(CMAKE_OSX_DEPLOYMENT_TARGET CACHE)

set(CMAKE_OSX_SYSROOT iphoneos)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf")

add_library(foo SHARED foo.cpp)
install(TARGETS foo DESTINATION lib)

set_target_properties(
  foo
  PROPERTIES
  XCODE_ATTRIBUTE_ARCHS[sdk=iphoneos*] armv7
  XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphoneos*] armv7
  XCODE_ATTRIBUTE_ARCHS[sdk=iphonesimulator*] ""
  XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphonesimulator*] ""
)
