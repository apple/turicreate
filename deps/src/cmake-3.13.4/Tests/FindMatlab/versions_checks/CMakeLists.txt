
cmake_minimum_required (VERSION 2.8.12)
enable_testing()
project(versions_checks)

set(MATLAB_FIND_DEBUG TRUE)
set(MATLAB_ADDITIONAL_VERSIONS
    "dummy=14.9")

if(NOT "${MCR_ROOT}" STREQUAL "")
    set(Matlab_ROOT_DIR "${MCR_ROOT}")
    if(NOT EXISTS "${MCR_ROOT}")
        message(FATAL_ERROR "MCR does not exist ${MCR_ROOT}")
    endif()
endif()

# the success of the following command is dependent on the current configuration
# in this case, we are only interested in the version macros
find_package(Matlab)



if(NOT COMMAND matlab_get_version_from_release_name)
  message(FATAL_ERROR "The macro matlab_get_version_from_release_name should be defined")
endif()

if(NOT COMMAND matlab_get_release_name_from_version)
  message(FATAL_ERROR "The macro matlab_get_release_name_from_version should be defined")
endif()


# matlab_get_release_name_from_version
matlab_get_release_name_from_version("7.13" release_name)
if(NOT release_name STREQUAL "R2011b")
  message(FATAL_ERROR "version 7.13 does not give release R2011b : '${release_name}' != R2011b")
endif()

matlab_get_release_name_from_version("14.9" release_name)
if(NOT release_name STREQUAL "dummy")
  message(FATAL_ERROR "version 14.9 does not give release dummy : '${release_name}' != dummy")
endif()

matlab_get_release_name_from_version("14.10" release_name)
if(NOT release_name STREQUAL "")
  message(FATAL_ERROR "version 14.10 does not give empty release: '${release_name}' != ''")
endif()


# matlab_get_version_from_release_name
matlab_get_version_from_release_name("R2011a" version)
if(NOT version STREQUAL "7.12")
  message(FATAL_ERROR "Release R2011a does not give version 7.12 : '${version}' != 7.12")
endif()

matlab_get_version_from_release_name("dummy" version)
#message(FATAL_ERROR "versionversion = ${version}")
if(NOT version STREQUAL "14.9")
  message(FATAL_ERROR "Release dummy does not give version 14.9 : '${version}' != 14.9")
endif()
