cmake_minimum_required(VERSION 2.8)
project(cse-snapshot)

include(ExternalProject)

include("${CMAKE_CURRENT_SOURCE_DIR}/LocalOverrides.cmake" OPTIONAL)
include("${CMAKE_CURRENT_BINARY_DIR}/LocalOverrides.cmake" OPTIONAL)

if(NOT DEFINED HOME)
  if(DEFINED ENV{CTEST_REAL_HOME})
    set(HOME "$ENV{CTEST_REAL_HOME}")
  else()
    set(HOME "$ENV{HOME}")
  endif()
endif()
message(STATUS "HOME='${HOME}'")

if(NOT DEFINED repo)
  set(repo "git://public.kitware.com/cse.git")
endif()
message(STATUS "repo='${repo}'")

if(NOT DEFINED tag)
  set(tag "cc1dcb95439a21ab1d58f444d93481598414196e")
endif()
message(STATUS "tag='${tag}'")

string(SUBSTRING "${tag}" 0 8 shorttag)

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

if(NOT DEFINED PROCESSOR_COUNT)
  # Unknown:
  set(PROCESSOR_COUNT 0)

  # Linux:
  set(cpuinfo_file "/proc/cpuinfo")
  if(EXISTS "${cpuinfo_file}")
    file(STRINGS "${cpuinfo_file}" procs REGEX "^processor.: [0-9]+$")
    list(LENGTH procs PROCESSOR_COUNT)
  endif()

  # Mac:
  if(APPLE)
    find_program(cmd_sysctl "sysctl")
    if(cmd_sysctl)
      execute_process(COMMAND ${cmd_sysctl} -n hw.ncpu
        OUTPUT_VARIABLE PROCESSOR_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
  endif()

  # Windows:
  if(WIN32)
    set(PROCESSOR_COUNT "$ENV{NUMBER_OF_PROCESSORS}")
  endif()
endif()
message(STATUS "PROCESSOR_COUNT='${PROCESSOR_COUNT}'")

find_package(Git)
if(NOT GIT_EXECUTABLE)
  message(FATAL_ERROR "error: could not find git")
  # adjust PATH to find git, or set GIT_EXECUTABLE in LocalOverrides.cmake
endif()
message(STATUS "GIT_EXECUTABLE='${GIT_EXECUTABLE}'")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Dashboard.cmake.in"
  "${script_dir}/Dashboard.cmake"
  @ONLY)

# Source dir for this project exists outside the CMake build tree because it
# is absolutely huge.
#
if(EXISTS "${source_dir}/.git")
  # If it exists already, download is a complete no-op:
  ExternalProject_Add(download-${PROJECT_NAME}
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
  )
else()
  # If it does not yet exist, download clones the git repository:
  ExternalProject_Add(download-${PROJECT_NAME}
    SOURCE_DIR "${source_dir}"
    GIT_REPOSITORY "${repo}"
    GIT_TAG "${tag}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
  )
endif()

ExternalProject_Add(build-${PROJECT_NAME}
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ${CMAKE_CTEST_COMMAND} -S "${script_dir}/Dashboard.cmake"
  INSTALL_COMMAND ""
  DEPENDS download-${PROJECT_NAME}
  )
