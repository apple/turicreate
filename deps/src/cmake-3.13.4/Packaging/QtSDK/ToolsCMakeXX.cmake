# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

# CMake version
include("${CMAKE_CURRENT_LIST_DIR}/../../Source/CMakeVersion.cmake")

# Install destinations
set(CMake_INSTALL_INFIX "Tools/CMake/${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}/"
  CACHE STRING "Location under install CMake tools")

# Package
set(CMake_IFW_ROOT_COMPONENT_NAME
  "qt.tools.cmake.${CMake_VERSION_MAJOR}${CMake_VERSION_MINOR}"
  CACHE STRING "QtSDK CMake tools component name")
set(CMake_IFW_ROOT_COMPONENT_DISPLAY_NAME
  "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}.${CMake_VERSION_PATCH}"
  CACHE STRING "QtSDK CMake tools component display name")
set(CMake_IFW_ROOT_COMPONENT_DESCRIPTION
  "CMake Build Tools ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}.${CMake_VERSION_PATH}"
  CACHE STRING "QtSDK CMake tools component description")
set(CMake_IFW_ROOT_COMPONENT_SCRIPT_TEMPLATE
  "${CMAKE_CURRENT_LIST_DIR}/qt.tools.cmake.xx.qs.in"
  CACHE FILEPATH "QtSDK CMake tools script template")
set(CMake_IFW_ROOT_COMPONENT_SCRIPT_GENERATED
  "${CMAKE_CURRENT_BINARY_DIR}/qt.tools.cmake.${CMake_VERSION_MAJOR}${CMake_VERSION_MINOR}.qs"
  CACHE FILEPATH "QtSDK CMake tools script generated")
set(CMake_IFW_ROOT_COMPONENT_PRIORITY
  "${CMake_VERSION_MAJOR}${CMake_VERSION_MINOR}"
  CACHE STRING "QtSDK CMake tools component sorting priority")
set(CMake_IFW_ROOT_COMPONENT_DEFAULT ""
  CACHE STRING "QtSDK CMake tools component default")
set(CMake_IFW_ROOT_COMPONENT_FORCED_INSTALLATION ""
  CACHE STRING "QtSDK CMake tools component forsed installation")

# CPack
set(CPACK_GENERATOR "IFW"
  CACHE STRING "Generator to build QtSDK CMake package")
set(CPACK_PACKAGE_FILE_NAME "CMake"
  CACHE STRING "Short package name")
set(CPACK_TOPLEVEL_TAG "../QtSDK"
  CACHE STRING "QtSDK packages dir")
set(CPACK_IFW_DOWNLOAD_ALL "TRUE"
  CACHE STRING "All QtSDK components is downloaded")
set(CPACK_DOWNLOAD_SITE "file:///${CMAKE_CURRENT_BINARY_DIR}/QtSDK/IFW/CMake/repository"
  CACHE STRING "Local repository for testing")
