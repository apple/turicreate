# Load version number components.
include(${CMake_SOURCE_DIR}/Source/CMakeVersion.cmake)

# Releases define a small patch level.
if("${CMake_VERSION_PATCH}" VERSION_LESS 20000000)
  set(CMake_VERSION_IS_DIRTY 0)
  set(CMake_VERSION_IS_RELEASE 1)
  set(CMake_VERSION_SOURCE "")
else()
  set(CMake_VERSION_IS_DIRTY 0) # may be set to 1 by CMakeVersionSource
  set(CMake_VERSION_IS_RELEASE 0)
  include(${CMake_SOURCE_DIR}/Source/CMakeVersionSource.cmake)
endif()

# Compute the full version string.
set(CMake_VERSION ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}.${CMake_VERSION_PATCH})
if(CMake_VERSION_SOURCE)
  set(CMake_VERSION_SUFFIX "${CMake_VERSION_SOURCE}")
elseif(CMake_VERSION_RC)
  set(CMake_VERSION_SUFFIX "rc${CMake_VERSION_RC}")
else()
  set(CMake_VERSION_SUFFIX "")
endif()
if(CMake_VERSION_SUFFIX)
  set(CMake_VERSION ${CMake_VERSION}-${CMake_VERSION_SUFFIX})
endif()
if(CMake_VERSION_IS_DIRTY)
  set(CMake_VERSION ${CMake_VERSION}-dirty)
endif()

# Compute the binary version that appears in the RC file. Version
# components in the RC file are 16-bit integers so we may have to
# split the patch component.
if(CMake_VERSION_PATCH MATCHES "^([0-9]+)([0-9][0-9][0-9][0-9])$")
  set(CMake_RCVERSION_YEAR "${CMAKE_MATCH_1}")
  set(CMake_RCVERSION_MONTH_DAY "${CMAKE_MATCH_2}")
  string(REGEX REPLACE "^0+" "" CMake_RCVERSION_MONTH_DAY "${CMake_RCVERSION_MONTH_DAY}")
  set(CMake_RCVERSION ${CMake_VERSION_MAJOR},${CMake_VERSION_MINOR},${CMake_RCVERSION_YEAR},${CMake_RCVERSION_MONTH_DAY})
  unset(CMake_RCVERSION_MONTH_DAY)
  unset(CMake_RCVERSION_YEAR)
else()
  set(CMake_RCVERSION ${CMake_VERSION_MAJOR},${CMake_VERSION_MINOR},${CMake_VERSION_PATCH})
endif()
set(CMake_RCVERSION_STR ${CMake_VERSION})
