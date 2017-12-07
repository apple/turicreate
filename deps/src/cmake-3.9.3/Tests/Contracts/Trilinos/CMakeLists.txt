cmake_minimum_required(VERSION 2.8)
project(Trilinos)

include(ExternalProject)

include("${CMAKE_CURRENT_SOURCE_DIR}/LocalOverrides.cmake" OPTIONAL)
include("${CMAKE_CURRENT_BINARY_DIR}/LocalOverrides.cmake" OPTIONAL)

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
message(STATUS "HOME='${HOME}'")

if(NOT DEFINED url)
  set(url "https://cmake.org/files/contracts/trilinos-11.4.1.tar.gz")
endif()
message(STATUS "url='${url}'")

if(NOT DEFINED md5)
  set(md5 "28b6a3c7c0fb317b3a237997293faa8b")
endif()
message(STATUS "md5='${md5}'")

string(SUBSTRING "${md5}" 0 8 shorttag)
set(shorttag "m${shorttag}")

set(download_dir "${HOME}/.cmake/Downloads")

set(base_dir "${HOME}/.cmake/Contracts/${PROJECT_NAME}/${shorttag}")
set(binary_dir "${base_dir}/build")
set(script_dir "${base_dir}")
set(source_dir "${base_dir}/src")

if(NOT DEFINED BUILDNAME)
  set(BUILDNAME "CMakeContract-${shorttag}")
endif()
message(STATUS "BUILDNAME='${BUILDNAME}'")

if(NOT DEFINED SITE)
  site_name(SITE)
endif()
message(STATUS "SITE='${SITE}'")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Dashboard.cmake.in"
  "${script_dir}/Dashboard.cmake"
  @ONLY)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/ValidateBuild.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ValidateBuild.cmake"
  @ONLY)

# Source dir for this project exists outside the CMake build tree because it
# is absolutely huge. Source dir is therefore cached under a '.cmake/Contracts'
# dir in your HOME directory. Downloads are cached under '.cmake/Downloads'
#
if(EXISTS "${source_dir}/cmake/ctest/TrilinosCTestDriverCore.cmake")
  # If it exists already, download is a complete no-op:
  ExternalProject_Add(download-${PROJECT_NAME}
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
  )
else()
  # If it does not yet exist, download pulls the tarball from the web (or
  # no-ops if it already exists with the given md5 sum):
  #
  ExternalProject_Add(download-${PROJECT_NAME}
    DOWNLOAD_DIR "${download_dir}"
    URL "${url}"
    URL_MD5 "${md5}"
    SOURCE_DIR "${source_dir}"
    PATCH_COMMAND ${CMAKE_COMMAND} -Dsource_dir=${source_dir} -P "${CMAKE_CURRENT_SOURCE_DIR}/Patch.cmake"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
  )
endif()

ExternalProject_Add(build-${PROJECT_NAME}
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ${CMAKE_COMMAND} -P "${script_dir}/Dashboard.cmake"
  INSTALL_COMMAND ""
  DEPENDS download-${PROJECT_NAME}
  )
