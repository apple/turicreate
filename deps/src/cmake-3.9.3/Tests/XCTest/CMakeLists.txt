cmake_minimum_required(VERSION 3.1)
project(XCTest)
enable_testing()

find_package(XCTest REQUIRED)

# Framework

add_library(FrameworkExample SHARED
  FrameworkExample/FrameworkExample.c
  FrameworkExample/FrameworkExample.h
  FrameworkExample/Info.plist)

target_include_directories(FrameworkExample PUBLIC .)

set_target_properties(FrameworkExample PROPERTIES
  FRAMEWORK TRUE
  VERSION "1.0.0"
  SOVERSION "1.0.0"
  FRAMEWORK_VERSION "A"
  MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/FrameworkExample/Info.plist
  PUBLIC_HEADER FrameworkExample/FrameworkExample.h)

# XCTest for Framework

xctest_add_bundle(FrameworkExampleTests FrameworkExample
  FrameworkExampleTests/FrameworkExampleTests.m
  FrameworkExampleTests/Info.plist)

set_target_properties(FrameworkExampleTests PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/FrameworkExampleTests/Info.plist
  )

xctest_add_test(XCTest.FrameworkExample FrameworkExampleTests)

# Cocoa App Bundle

add_executable(CocoaExample MACOSX_BUNDLE
  CocoaExample/main.m
  CocoaExample/AppDelegate.m
  CocoaExample/AppDelegate.h
  CocoaExample/MainMenu.xib
)

target_link_libraries(CocoaExample PRIVATE "-framework Foundation")
target_link_libraries(CocoaExample PRIVATE "-framework AppKit")

set_target_properties(CocoaExample PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/CocoaExample/Info.plist
  RESOURCE "CocoaExample/MainMenu.xib")

# XCTest for Cocoa App Bundle

xctest_add_bundle(CocoaExampleTests CocoaExample
  CocoaExampleTests/CocoaExampleTests.m)

xctest_add_test(XCTest.CocoaExample CocoaExampleTests)

# Static lib

add_library(StaticLibExample STATIC
  StaticLibExample/StaticLibExample.h
  StaticLibExample/StaticLibExample.c
)

target_include_directories(StaticLibExample PUBLIC .)

# XCTest for Static lib

xctest_add_bundle(StaticLibExampleTests StaticLibExample
  StaticLibExampleTests/StaticLibExampleTests.m)

xctest_add_test(XCTest.StaticLibExample StaticLibExampleTests)
