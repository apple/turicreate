#this is adapted from FireBreath (http://www.firebreath.org)

cmake_minimum_required(VERSION 2.8)

project(CFBundleTest)

include(PluginConfig.cmake)

message ("Creating Mac Browser Plugin project ${PROJECT_NAME}")
set(SOURCES
    np_macmain.cpp
    Localized.r
    ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
    ${CMAKE_CURRENT_BINARY_DIR}/InfoPlist.strings
    ${CMAKE_CURRENT_BINARY_DIR}/Localized.rsrc
)

add_library( ${PROJECT_NAME} MODULE
    ${SOURCES}
    )

set (RCFILES ${CMAKE_CURRENT_SOURCE_DIR}/Localized.r)

configure_file(Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
configure_file(InfoPlist.strings.in ${CMAKE_CURRENT_BINARY_DIR}/InfoPlist.strings)

# Compile the resource file
find_program(RC_COMPILER Rez NO_DEFAULT_PATHS PATHS /Developer/Tools)
if(NOT RC_COMPILER)
  message(FATAL_ERROR "could not find Rez to build resources from .r file...")
endif()

set(sysroot)
if(CMAKE_OSX_SYSROOT)
  set(sysroot -isysroot ${CMAKE_OSX_SYSROOT})
endif()

execute_process(COMMAND
    ${RC_COMPILER} ${sysroot} ${RCFILES} -useDF
    -o ${CMAKE_CURRENT_BINARY_DIR}/Localized.rsrc
    )

set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/Localized.rsrc
    PROPERTIES GENERATED 1
    )
# note that for some reason, the makefile and xcode generators use a different
# property to indicate where the Info.plist file is :-/ For that reason, we
# specify it twice so it will work both places
set_target_properties(CFBundleTest PROPERTIES
    BUNDLE 1
    BUNDLE_EXTENSION plugin
    XCODE_ATTRIBUTE_MACH_O_TYPE mh_bundle
    XCODE_ATTRIBUTE_INFOPLIST_FILE ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
    LINK_FLAGS "-Wl,-exported_symbols_list,\"${CMAKE_CURRENT_SOURCE_DIR}/ExportList_plugin.txt\"")

set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/InfoPlist.strings
    ${CMAKE_CURRENT_BINARY_DIR}/Localized.rsrc
    PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/English.lproj")

export(TARGETS CFBundleTest FILE ${CMAKE_CURRENT_BINARY_DIR}/exp.cmake)
