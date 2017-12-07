cmake_minimum_required (VERSION 2.6)
project(BundleTest)
set(MACOSX_BUNDLE_INFO_STRING "bundle_info_string")
set(CMAKE_MacOSX_Content_COMPILE_OBJECT "\"${CMAKE_COMMAND}\" -E copy_if_different <SOURCE> <OBJECT>")

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/randomResourceFile.plist"
  COMMAND /bin/cp
  ARGS "${CMAKE_CURRENT_SOURCE_DIR}/randomResourceFile.plist.in"
  "${CMAKE_CURRENT_BINARY_DIR}/randomResourceFile.plist")

set_source_files_properties(
  "${CMAKE_CURRENT_BINARY_DIR}/randomResourceFile.plist"
  PROPERTIES
  MACOSX_PACKAGE_LOCATION Resources
  )

set_source_files_properties(
  SomeRandomFile.txt
  "${BundleTest_SOURCE_DIR}/../../README.rst"
  PROPERTIES
  MACOSX_PACKAGE_LOCATION MacOS
  )

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/foobar")

# Test building a bundle linking to a shared library where the
# shared library links to CoreFoundation, but the executable does not
# explicitly link to CoreFoundation, but the executable does *depend*
# on CoreFoundation. There should be a link failure for the executable
# if CMake's dependency chaining for libraries with "-framework
# blah" style dependencies gets broken...
#
add_library(BundleTestLib SHARED BundleLib.cxx)
target_link_libraries(BundleTestLib "-framework CoreFoundation")

add_executable(BundleTest
  MACOSX_BUNDLE
  BundleTest.cxx
  SomeRandomFile.txt
  "${BundleTest_SOURCE_DIR}/../../README.rst"
  "${CMAKE_CURRENT_BINARY_DIR}/randomResourceFile.plist"
  )
target_link_libraries(BundleTest BundleTestLib)
#
# DO NOT: target_link_libraries(BundleTest "-framework CoreFoundation")
#   (see above comments about CoreFoundation)
#

# Test bundle installation.
#install(TARGETS BundleTestLib DESTINATION Applications/BundleTestExe.app/Contents/Plugins)
install(TARGETS BundleTestLib DESTINATION Applications/SecondBundleExe.app/Contents/Plugins)
install(TARGETS BundleTest DESTINATION Applications)

# Test whether bundles respect the output name.  Since the library is
# installed into a location that uses this output name this will fail if the
# bundle does not respect the name.  Also the executable will not be found by
# the test driver if this does not work.
set_target_properties(BundleTest PROPERTIES OUTPUT_NAME BundleTestExe)

# Test executable versioning if it is supported.
if(NOT XCODE)
  set_target_properties(BundleTest PROPERTIES VERSION 1)
endif()

# Make sure the executable can find its installed library.
set_target_properties(BundleTestLib PROPERTIES
  INSTALL_NAME_DIR "@executable_path/../Plugins")

include(CPack)

# test the framework find stuff
if(EXISTS /usr/lib/libtcl.dylib
    AND EXISTS /System/Library/Frameworks/Tcl.framework)
  set(TCL NOTFOUND)
  find_library(TCL tcl)
  message("frame: ${TCL}")
  if(NOT "${TCL}" MATCHES .framework)
    message(FATAL_ERROR "Could not find tcl framework, found ${TCL}")
  endif()
  set(TCL NOTFOUND)
  set(CMAKE_FIND_FRAMEWORK LAST)
  find_library(TCL tcl)
  if("${TCL}" MATCHES .framework)
    message(FATAL_ERROR "Found framework and should have found dylib ${TCL}")
  endif()
  set(TCL NOTFOUND)
  set(CMAKE_FIND_FRAMEWORK NEVER)
  find_library(TCL tcl)
  if("${TCL}" MATCHES .framework)
    message(FATAL_ERROR "Found framework and should have found dylib ${TCL}")
  endif()
  message("not frame: ${TCL}")
  set(TCL NOTFOUND)
  set(CMAKE_FIND_FRAMEWORK FIRST)
  find_library(TCL tcl)
  if(NOT "${TCL}" MATCHES .framework)
    message(FATAL_ERROR "Could not find tcl framework, found ${TCL}")
  endif()
  message("frame: ${TCL}")
endif(EXISTS /usr/lib/libtcl.dylib
  AND EXISTS /System/Library/Frameworks/Tcl.framework)

subdirs(BundleSubDir)
