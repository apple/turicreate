cmake_minimum_required(VERSION 3.12)

project(FindPkgConfig_IMPORTED_TARGET C)

find_package(PkgConfig REQUIRED)

message(STATUS "source: ${CMAKE_CURRENT_SOURCE_DIR} bin ${CMAKE_CURRENT_BINARY_DIR}")

# Setup for the remaining package tests below
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH)
set(fakePkgDir ${CMAKE_CURRENT_BINARY_DIR}/pc-fakepackage)
file(WRITE ${fakePkgDir}/lib/libcmakeinternalfakepackage.a "")
file(WRITE ${fakePkgDir}/lib/cmakeinternalfakepackage.lib  "")
file(WRITE ${fakePkgDir}/lib/pkgconfig/cmakeinternalfakepackage.pc
"Name: CMakeInternalFakePackage
Description: Dummy package for FindPkgConfig VERSION_OPERATORS test
Version: 8.9
Libs: -lcmakeinternalfakepackage
")

# Always find the .pc file in the calls further below so that we can test that
# the import target find_library() calls handle the NO...PATH options correctly
set(ENV{PKG_CONFIG_PATH} ${fakePkgDir}/lib/pkgconfig)

pkg_check_modules(FakePackageGE REQUIRED QUIET "cmakeinternalfakepackage >= 8")
if (NOT FakePackageGE_FOUND)
  message(FATAL_ERROR "fake package >= 8 not found")
endif()

pkg_check_modules(FakePackageGE_FAIL QUIET "cmakeinternalfakepackage >= 8.10")
if (FakePackageGE_FAIL_FOUND)
  message(FATAL_ERROR "fake package >= 8.10 found")
endif()

pkg_check_modules(FakePackageLE REQUIRED QUIET "cmakeinternalfakepackage<=9")
if (NOT FakePackageLE_FOUND)
  message(FATAL_ERROR "fake package <= 9 not found")
endif()

pkg_check_modules(FakePackageLE_FAIL QUIET "cmakeinternalfakepackage <= 8.1")
if (FakePackageLE_FAIL_FOUND)
  message(FATAL_ERROR "fake package <= 8.1 found")
endif()

pkg_check_modules(FakePackageGT REQUIRED QUIET "cmakeinternalfakepackage > 8")
if (NOT FakePackageGT_FOUND)
  message(FATAL_ERROR "fake package > 8 not found")
endif()

pkg_check_modules(FakePackageGT_FAIL QUIET "cmakeinternalfakepackage > 8.9")
if (FakePackageGT_FAIL_FOUND)
  message(FATAL_ERROR "fake package > 8.9 found")
endif()

pkg_check_modules(FakePackageLT REQUIRED QUIET "cmakeinternalfakepackage<9")
if (NOT FakePackageLT_FOUND)
  message(FATAL_ERROR "fake package < 9 not found")
endif()

pkg_check_modules(FakePackageLT_FAIL QUIET "cmakeinternalfakepackage < 8.9")
if (FakePackageLT_FAIL_FOUND)
  message(FATAL_ERROR "fake package < 8.9 found")
endif()

pkg_check_modules(FakePackageEQ REQUIRED QUIET "cmakeinternalfakepackage=8.9")
if (NOT FakePackageEQ_FOUND)
  message(FATAL_ERROR "fake package = 8.9 not found")
endif()

pkg_check_modules(FakePackageEQ_FAIL QUIET "cmakeinternalfakepackage = 8.8")
if (FakePackageEQ_FAIL_FOUND)
  message(FATAL_ERROR "fake package = 8.8 found")
endif()

pkg_check_modules(FakePackageEQ_INV QUIET "cmakeinternalfakepackage == 8.9")
if (FakePackageEQ_FAIL_FOUND)
  message(FATAL_ERROR "fake package == 8.9 found")
endif()

pkg_check_modules(FakePackageLLT_INV QUIET "cmakeinternalfakepackage <<= 9")
if (FakePackageLLT_FAIL_FOUND)
  message(FATAL_ERROR "fake package <<= 9 found")
endif()
