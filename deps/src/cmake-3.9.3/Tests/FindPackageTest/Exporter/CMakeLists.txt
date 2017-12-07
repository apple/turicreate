cmake_minimum_required(VERSION 2.7.20090831)
project(CMakeTestExportPackage C)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMakeTestExportPackageConfig.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/CMakeTestExportPackageConfig.cmake
               @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMakeTestExportPackageConfigVersion.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/CMakeTestExportPackageConfigVersion.cmake
               @ONLY)
export(PACKAGE CMakeTestExportPackage)

add_executable(dummy dummy.c)
