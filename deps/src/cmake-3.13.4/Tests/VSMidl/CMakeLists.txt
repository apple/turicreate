# This CMakeLists.txt file exists solely to drive the one found in the "src"
# subdir as an ExternalProject build. The project in "src" cannot build when
# there is a space in the directory name, so we copy that directory to a place
# guaranteed not to have a space in the name, build it there, and then copy the
# resulting output directory back up here into this CMake test's build tree.
#
if(NOT DEFINED CMAKE_BUILDNAME)
  string(REGEX REPLACE "^.*/([^/]+)/[^/]+/([^/]+)$" "\\1" CMAKE_BUILDNAME "${CMAKE_CURRENT_BINARY_DIR}")
  string(REGEX REPLACE "^.*/([^/]+)/[^/]+/([^/]+)$" "\\2" THIS_TESTNAME "${CMAKE_CURRENT_BINARY_DIR}")
  string(REPLACE " " "_" CMAKE_BUILDNAME "${CMAKE_BUILDNAME}")
endif()
message(STATUS "CMAKE_BUILDNAME='${CMAKE_BUILDNAME}'")
message(STATUS "THIS_TESTNAME='${THIS_TESTNAME}'")

cmake_minimum_required(VERSION 2.8)
project(${THIS_TESTNAME})

include(ExternalProject)

if(NOT DEFINED HOME)
  if(DEFINED ENV{CTEST_REAL_HOME})
    set(HOME "$ENV{CTEST_REAL_HOME}")
  else()
    set(HOME "$ENV{HOME}")
  endif()

  if(NOT HOME AND WIN32)
    # Try for USERPROFILE as HOME equivalent:
    string(REPLACE "\\" "/" HOME "$ENV{USERPROFILE}")
  endif()

  # But just use root of SystemDrive if HOME contains any spaces:
  # (Default on XP and earlier...)
  if(HOME MATCHES " " AND WIN32)
    string(REPLACE "\\" "/" HOME "$ENV{SystemDrive}")
  endif()
  if(HOME MATCHES " ")
    set(HOME "")
  endif()
endif()
message(STATUS "HOME='${HOME}'")

if(NOT DEFINED url)
  set(url "${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()
message(STATUS "url='${url}'")

set(base_dir "${HOME}/.cmake/Dashboards/${CMAKE_BUILDNAME}/${THIS_TESTNAME}")
set(binary_dir "${base_dir}/build")
set(source_dir "${base_dir}/src")

# Source dir for this project exists in the CMake source tree, but we cannot
# use it in-place since there might be a space in its directory name.
# Source dir is therefore copied under a '.cmake/Dashboards'
# dir in your HOME directory to give it a name with no spaces.
#
ExternalProject_Add(clean-${PROJECT_NAME}
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory "${source_dir}"
  BUILD_COMMAND ${CMAKE_COMMAND} -E remove_directory "${binary_dir}"
  INSTALL_COMMAND ""
  )

ExternalProject_Add(download-${PROJECT_NAME}
  URL "${url}"
  SOURCE_DIR "${source_dir}"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  DEPENDS clean-${PROJECT_NAME}
  )

ExternalProject_Add(build-${PROJECT_NAME}
  DOWNLOAD_COMMAND ""
  SOURCE_DIR "${source_dir}"
  BINARY_DIR "${binary_dir}"
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${binary_dir}/${CMAKE_CFG_INTDIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}"
  DEPENDS download-${PROJECT_NAME}
  )
