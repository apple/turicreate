cmake_minimum_required(VERSION 2.8.5)
project(NavApp3)

set(CMAKE_OSX_SYSROOT iphoneos4.3)
set(CMAKE_OSX_ARCHITECTURES "armv6;armv7;i386")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Classes
  )

add_library(Functions STATIC TotalFunction.c TotalFunction.h)

set(M_SRCS main.m Classes/NavApp3AppDelegate.m Classes/RootViewController.m)
set(HEADERS Classes/NavApp3AppDelegate.h Classes/RootViewController.h)
set(RESOURCES MainWindow.xib RootViewController.xib)

add_executable(NavApp3 MACOSX_BUNDLE ${M_SRCS} ${HEADERS} ${RESOURCES})

target_link_libraries(NavApp3
  Functions
  "-framework CoreGraphics"
  "-framework Foundation"
  "-framework UIKit"
  )

set_target_properties(NavApp3 PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.NavApp3"
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
  RESOURCE "${RESOURCES}"
  XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
  XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
  XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES
  XCODE_ATTRIBUTE_GCC_PREFIX_HEADER ${CMAKE_CURRENT_LIST_DIR}/NavApp3_Prefix.pch
  XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES
  XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 3.0
  )
