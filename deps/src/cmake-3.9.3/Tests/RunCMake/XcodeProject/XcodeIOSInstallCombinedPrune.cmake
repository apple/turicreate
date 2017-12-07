cmake_minimum_required(VERSION 3.3)

project(XcodeIOSInstallCombinedPrune CXX)

set(CMAKE_OSX_SYSROOT iphoneos)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf")

add_library(foo SHARED foo.cpp)
install(TARGETS foo DESTINATION lib)

add_library(baz SHARED foo.cpp)
set_target_properties(
  foo baz
  PROPERTIES
  XCODE_ATTRIBUTE_ARCHS[sdk=iphoneos*] armv7
  XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphoneos*] armv7
  XCODE_ATTRIBUTE_ARCHS[sdk=iphonesimulator*] x86_64
  XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphonesimulator*] x86_64
)

add_library(boo SHARED foo.cpp)
set_target_properties(
  boo
  PROPERTIES
  XCODE_ATTRIBUTE_ARCHS[sdk=iphoneos*] arm64
  XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphoneos*] arm64
  XCODE_ATTRIBUTE_ARCHS[sdk=iphonesimulator*] i386
  XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphonesimulator*] i386
)

add_custom_command(
  TARGET foo
  POST_BUILD
  COMMAND lipo -create $<TARGET_FILE:baz> $<TARGET_FILE:boo> -output $<TARGET_FILE:foo>
)
