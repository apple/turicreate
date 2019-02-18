cmake_minimum_required(VERSION 3.12)

project(FindPkgConfig_IMPORTED_TARGET C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES IMPORTED_TARGET QUIET ncurses)

message(STATUS "source: ${CMAKE_CURRENT_SOURCE_DIR} bin ${CMAKE_CURRENT_BINARY_DIR}")

if (NCURSES_FOUND)
  set(tgt PkgConfig::NCURSES)
  if (NOT TARGET ${tgt})
    message(FATAL_ERROR "FindPkgConfig found ncurses, but did not create an imported target for it")
  endif ()
  set(prop_found FALSE)
  foreach (prop IN ITEMS INTERFACE_INCLUDE_DIRECTORIES INTERFACE_LINK_LIBRARIES INTERFACE_COMPILE_OPTIONS)
    get_target_property(value ${tgt} ${prop})
    if (value)
      message(STATUS "Found property ${prop} on target: ${value}")
      set(prop_found TRUE)
    endif ()
  endforeach ()
  if (NOT prop_found)
    message(FATAL_ERROR "target ${tgt} found, but it has no properties")
  endif ()
else ()
  message(STATUS "skipping test; ncurses not found")
endif ()


# Setup for the remaining package tests below
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH)
set(fakePkgDir ${CMAKE_CURRENT_BINARY_DIR}/pc-fakepackage)
foreach(i 1 2)
  set(pname cmakeinternalfakepackage${i})
  file(WRITE ${fakePkgDir}/lib/lib${pname}.a "")
  file(WRITE ${fakePkgDir}/lib/${pname}.lib  "")
  file(WRITE ${fakePkgDir}/lib/pkgconfig/${pname}.pc
"Name: CMakeInternalFakePackage${i}
Description: Dummy package (${i}) for FindPkgConfig IMPORTED_TARGET test
Version: 1.2.3
Libs: -l${pname}
")
endforeach()

# Always find the .pc file in the calls further below so that we can test that
# the import target find_library() calls handle the NO...PATH options correctly
set(ENV{PKG_CONFIG_PATH} ${fakePkgDir}/lib/pkgconfig)

# Confirm correct behavior of NO_CMAKE_PATH, ensuring we only find the library
# for the imported target if we have both set CMAKE_PREFIX_PATH and have not
# given the NO_CMAKE_PATH option
unset(CMAKE_PREFIX_PATH)
unset(ENV{CMAKE_PREFIX_PATH})
pkg_check_modules(FakePackage1 QUIET IMPORTED_TARGET cmakeinternalfakepackage1)
if (TARGET PkgConfig::FakePackage1)
  message(FATAL_ERROR "Have import target for fake package 1 with no path prefix")
endif()

set(CMAKE_PREFIX_PATH ${fakePkgDir})
pkg_check_modules(FakePackage1 QUIET IMPORTED_TARGET NO_CMAKE_PATH cmakeinternalfakepackage1)
if (TARGET PkgConfig::FakePackage1)
  message(FATAL_ERROR "Have import target for fake package 1 with ignored cmake path")
endif()

pkg_check_modules(FakePackage1 REQUIRED QUIET IMPORTED_TARGET cmakeinternalfakepackage1)
if (NOT TARGET PkgConfig::FakePackage1)
  message(FATAL_ERROR "No import target for fake package 1 with prefix path")
endif()

# find targets in subdir and check their visibility
add_subdirectory(target_subdir)
if (TARGET PkgConfig::FakePackage1_dir)
  message(FATAL_ERROR "imported target PkgConfig::FakePackage1_dir is visible outside it's directory")
endif()

if (NOT TARGET PkgConfig::FakePackage1_global)
  message(FATAL_ERROR "imported target PkgConfig::FakePackage1_global is not visible outside it's directory")
endif()

# And now do the same for the NO_CMAKE_ENVIRONMENT_PATH - ENV{CMAKE_PREFIX_PATH}
# combination
unset(CMAKE_PREFIX_PATH)
unset(ENV{CMAKE_PREFIX_PATH})
pkg_check_modules(FakePackage2 QUIET IMPORTED_TARGET cmakeinternalfakepackage2)
if (TARGET PkgConfig::FakePackage2)
  message(FATAL_ERROR "Have import target for fake package 2 with no path prefix")
endif()

set(ENV{CMAKE_PREFIX_PATH} ${fakePkgDir})
pkg_check_modules(FakePackage2 QUIET IMPORTED_TARGET NO_CMAKE_ENVIRONMENT_PATH cmakeinternalfakepackage2)
if (TARGET PkgConfig::FakePackage2)
  message(FATAL_ERROR "Have import target for fake package 2 with ignored cmake path")
endif()

pkg_check_modules(FakePackage2 REQUIRED QUIET IMPORTED_TARGET cmakeinternalfakepackage2)
if (NOT TARGET PkgConfig::FakePackage2)
  message(FATAL_ERROR "No import target for fake package 2 with prefix path")
endif()

# check that the full library path is also returned
if (NOT FakePackage2_LINK_LIBRARIES STREQUAL "${fakePkgDir}/lib/libcmakeinternalfakepackage2.a")
  message(FATAL_ERROR "FakePackage2_LINK_LIBRARIES has bad content on first run: ${FakePackage2_LINK_LIBRARIES}")
endif()

# the information in *_LINK_LIBRARIES is not cached, so ensure is also is present on second run
unset(FakePackage2_LINK_LIBRARIES)
pkg_check_modules(FakePackage2 REQUIRED QUIET IMPORTED_TARGET cmakeinternalfakepackage2)
if (NOT FakePackage2_LINK_LIBRARIES STREQUAL "${fakePkgDir}/lib/libcmakeinternalfakepackage2.a")
  message(FATAL_ERROR "FakePackage2_LINK_LIBRARIES has bad content on second run: ${FakePackage2_LINK_LIBRARIES}")
endif()
