# The VTK external project for CMake
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8)
project(VTK)
include(ExternalProject)

# find "HOME".  VTK will be downloaded & built within a subdirectory.
if(NOT DEFINED HOME)
  if(DEFINED ENV{CTEST_REAL_HOME})
    set(HOME "$ENV{CTEST_REAL_HOME}")
  else()
    set(HOME "$ENV{HOME}")
  endif()

  if(NOT HOME AND WIN32)
    # Try for USERPROFILE as HOME equivalent:
    string(REPLACE "\\" "/" HOME "$ENV{USERPROFILE}")

    # But just use root of SystemDrive if USERPROFILE contains any spaces:
    # (Default on XP and earlier...)
    if(HOME MATCHES " ")
      string(REPLACE "\\" "/" HOME "$ENV{SystemDrive}")
    endif()
  endif()
endif()

set(base_dir "${HOME}/.cmake/Contracts/VTK")

if(NOT DEFINED SITE)
  site_name(SITE)
endif()

# configure our dashboard script
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Dashboard.cmake.in
  ${base_dir}/Dashboard.cmake
  @ONLY)

# build & test VTK's release branch
ExternalProject_Add(${PROJECT_NAME}
  GIT_REPOSITORY "git://vtk.org/VTK.git"
  GIT_TAG "release"
  PREFIX ${base_dir}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ${CMAKE_CTEST_COMMAND} -S "${base_dir}/Dashboard.cmake"
  INSTALL_COMMAND ""
)
