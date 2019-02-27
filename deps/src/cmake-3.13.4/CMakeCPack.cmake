# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

option(CMAKE_INSTALL_DEBUG_LIBRARIES
  "Install Microsoft runtime debug libraries with CMake." FALSE)
mark_as_advanced(CMAKE_INSTALL_DEBUG_LIBRARIES)

# By default, do not warn when built on machines using only VS Express:
if(NOT DEFINED CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS ON)
endif()

if(CMake_INSTALL_DEPENDENCIES)
  include(${CMake_SOURCE_DIR}/Modules/InstallRequiredSystemLibraries.cmake)
endif()

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CMake is a build tool")
set(CPACK_PACKAGE_VENDOR "Kitware")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt")
set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${CMake_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "cmake-${CMake_VERSION}")

# Installers for 32- vs. 64-bit CMake:
#  - Root install directory (displayed to end user at installer-run time)
#  - "NSIS package/display name" (text used in the installer GUI)
#  - Registry key used to store info about the installation
if(CMAKE_CL_64)
  set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
  set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION} (Win64)")
else()
  set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
  set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
endif()
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_NSIS_PACKAGE_NAME}")

if(NOT DEFINED CPACK_SYSTEM_NAME)
  # make sure package is not Cygwin-unknown, for Cygwin just
  # cygwin is good for the system name
  if("x${CMAKE_SYSTEM_NAME}" STREQUAL "xCYGWIN")
    set(CPACK_SYSTEM_NAME Cygwin)
  else()
    set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
  endif()
endif()
if(${CPACK_SYSTEM_NAME} MATCHES Windows)
  if(CMAKE_CL_64)
    set(CPACK_SYSTEM_NAME win64-x64)
    set(CPACK_IFW_TARGET_DIRECTORY "@RootDir@/Program Files/${CMAKE_PROJECT_NAME}")
  else()
    set(CPACK_SYSTEM_NAME win32-x86)
  endif()
endif()

# Command for configure IFW script templates
include(${CMake_SOURCE_DIR}/Modules/CPackIFWConfigureFile.cmake)

# Advanced IFW configuration
set(_cpifwrc CPACK_IFW_COMPONENT_GROUP_CMAKE_)
set(_cpifwrcconf _CPACK_IFW_COMPONENT_GROUP_CMAKE)
set(${_cpifwrcconf} "# CMake IFW configuration\n")
macro(_cmifwarg DESCRIPTION TYPE NAME DEFAULT)
  set(_var CMake_IFW_ROOT_COMPONENT_${NAME})
  if(DEFINED ${_var})
    set(${_var} ${${_var}} CACHE ${TYPE} ${DESCRIPTION})
    mark_as_advanced(${_var})
  elseif(NOT "${DEFAULT}" STREQUAL "")
    set(${_var} ${DEFAULT})
  endif()
  if(DEFINED ${_var})
    set(${_cpifwrcconf}
      "${${_cpifwrcconf}}  set(${_cpifwrc}${NAME}\n    \"${${_var}}\")\n")
  endif()
endmacro()

_cmifwarg("Package <Name> tag (domen-like)"
  STRING NAME "")
_cmifwarg("Package <DisplayName> tag"
  STRING DISPLAY_NAME "")
_cmifwarg("Package <Description> tag"
  STRING DESCRIPTION "")
_cmifwarg("Package <ReleaseDate> tag (keep empty to auto generate)"
  STRING RELEASE_DATE "")
_cmifwarg("Package <Default> tag (values: TRUE, FALSE, SCRIPT)"
  STRING DEFAULT "")
_cmifwarg("Package <Version> tag"
  STRING VERSION
  "${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}.${CMake_VERSION_PATCH}")
_cmifwarg("Package <SortingPriority> tag"
  STRING PRIORITY "100")
_cmifwarg("Package <ForsedInstallation> tag"
  STRING FORCED_INSTALLATION "")

set(${_cpifwrc}LICENSES_DEFAULT
  "${CPACK_PACKAGE_NAME} Copyright;${CPACK_RESOURCE_FILE_LICENSE}")

# Components
if(CMake_INSTALL_COMPONENTS)
  set(_CPACK_IFW_COMPONENTS_ALL cmake ctest cpack)
  if(WIN32 AND NOT CYGWIN)
      list(APPEND _CPACK_IFW_COMPONENTS_ALL cmcldeps)
  endif()
  if(APPLE)
    list(APPEND _CPACK_IFW_COMPONENTS_ALL cmakexbuild)
  endif()
  if(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME)
    set(_CPACK_IFW_COMPONENT_UNSPECIFIED_NAME
      ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME})
  else()
    set(_CPACK_IFW_COMPONENT_UNSPECIFIED_NAME Unspecified)
  endif()
  list(APPEND _CPACK_IFW_COMPONENTS_ALL ${_CPACK_IFW_COMPONENT_UNSPECIFIED_NAME})
  string(TOUPPER "${_CPACK_IFW_COMPONENT_UNSPECIFIED_NAME}"
    _CPACK_IFW_COMPONENT_UNSPECIFIED_UNAME)
  if(BUILD_CursesDialog)
    list(APPEND _CPACK_IFW_COMPONENTS_ALL ccmake)
  endif()
  if(BUILD_QtDialog)
    list(APPEND _CPACK_IFW_COMPONENTS_ALL cmake-gui)
    if(USE_LGPL)
      set(_CPACK_IFW_COMPONENT_CMAKE-GUI_LICENSES "set(CPACK_IFW_COMPONENT_CMAKE-GUI_LICENSES
  \"LGPLv${USE_LGPL}\" \"${CMake_SOURCE_DIR}/Licenses/LGPLv${USE_LGPL}.txt\")")
    endif()
  endif()
  if(SPHINX_MAN)
    list(APPEND _CPACK_IFW_COMPONENTS_ALL sphinx-man)
  endif()
  if(SPHINX_HTML)
    list(APPEND _CPACK_IFW_COMPONENTS_ALL sphinx-html)
  endif()
  if(SPHINX_SINGLEHTML)
    list(APPEND _CPACK_IFW_COMPONENTS_ALL sphinx-singlehtml)
  endif()
  if(SPHINX_QTHELP)
    list(APPEND _CPACK_IFW_COMPONENTS_ALL sphinx-qthelp)
  endif()
  if(CMake_BUILD_DEVELOPER_REFERENCE)
    if(CMake_BUILD_DEVELOPER_REFERENCE_HTML)
      list(APPEND _CPACK_IFW_COMPONENTS_ALL cmake-developer-reference-html)
    endif()
    if(CMake_BUILD_DEVELOPER_REFERENCE_QTHELP)
      list(APPEND _CPACK_IFW_COMPONENTS_ALL cmake-developer-reference-qthelp)
    endif()
  endif()
  set(_CPACK_IFW_COMPONENTS_CONFIGURATION "
  # Components
  set(CPACK_COMPONENTS_ALL \"${_CPACK_IFW_COMPONENTS_ALL}\")
  set(CPACK_COMPONENTS_GROUPING IGNORE)
  ")
  _cmifwarg("Package <Script> template"
    FILEPATH SCRIPT_TEMPLATE "${CMake_SOURCE_DIR}/Source/QtIFW/CMake.qs.in")
else()
  if(BUILD_QtDialog AND USE_LGPL)
    set(${_cpifwrc}LICENSES_DEFAULT
      "${${_cpifwrc}LICENSES_DEFAULT};LGPLv${USE_LGPL};${CMake_SOURCE_DIR}/Licenses/LGPLv${USE_LGPL}.txt")
  endif()
  _cmifwarg("Package <Script> template"
    FILEPATH SCRIPT_TEMPLATE "${CMake_SOURCE_DIR}/Source/QtIFW/installscript.qs.in")
endif()
_cmifwarg("Package <Script> generated"
  FILEPATH SCRIPT_GENERATED "${CMake_BINARY_DIR}/CMake.qs")

_cmifwarg("Package <Licenses> tag (pairs of <display_name> <file_path>)"
  STRING LICENSES "${${_cpifwrc}LICENSES_DEFAULT}")

if(${CMAKE_SYSTEM_NAME} MATCHES Windows)
  set(_CPACK_IFW_PACKAGE_ICON
      "set(CPACK_IFW_PACKAGE_ICON \"${CMake_SOURCE_DIR}/Source/QtDialog/CMakeSetup.ico\")")
  if(BUILD_QtDialog)
    set(_CPACK_IFW_SHORTCUT_OPTIONAL "${_CPACK_IFW_SHORTCUT_OPTIONAL}component.addOperation(\"CreateShortcut\", \"@TargetDir@/bin/cmake-gui.exe\", \"@StartMenuDir@/CMake (cmake-gui).lnk\");\n")
  endif()
  if(SPHINX_HTML)
    set(_CPACK_IFW_SHORTCUT_OPTIONAL "${_CPACK_IFW_SHORTCUT_OPTIONAL}component.addOperation(\"CreateShortcut\", \"@TargetDir@/doc/cmake-${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}/html/index.html\", \"@StartMenuDir@/CMake Documentation.lnk\");\n")
  endif()
  if(CMake_BUILD_DEVELOPER_REFERENCE)
    if(CMake_BUILD_DEVELOPER_REFERENCE_HTML)
    set(_CPACK_IFW_SHORTCUT_OPTIONAL "${_CPACK_IFW_SHORTCUT_OPTIONAL}component.addOperation(\"CreateShortcut\", \"@TargetDir@/doc/cmake-${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}/developer-reference/html/index.html\", \"@StartMenuDir@/CMake Developer Reference.lnk\");\n")
    endif()
  endif()
  install(FILES "${CMake_SOURCE_DIR}/Source/QtIFW/cmake.org.html"
    DESTINATION "${CMAKE_DOC_DIR}"
  )
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
  set(CPACK_IFW_TARGET_DIRECTORY "@HomeDir@/${CMAKE_PROJECT_NAME}")
  set(CPACK_IFW_ADMIN_TARGET_DIRECTORY "@ApplicationsDir@/${CMAKE_PROJECT_NAME}")
endif()

# Components scripts configuration
if((EXISTS "${CMake_IFW_ROOT_COMPONENT_SCRIPT_TEMPLATE}")
  AND (NOT "${CMake_IFW_ROOT_COMPONENT_SCRIPT_GENERATED}" STREQUAL "")
  AND (NOT "${CMake_IFW_ROOT_COMPONENT_SCRIPT}"))
  cpack_ifw_configure_file("${CMake_IFW_ROOT_COMPONENT_SCRIPT_TEMPLATE}"
    "${CMake_IFW_ROOT_COMPONENT_SCRIPT_GENERATED}")
  _cmifwarg("Package <Script> tag"
    FILEPATH SCRIPT "${CMake_IFW_ROOT_COMPONENT_SCRIPT_GENERATED}")
endif()
foreach(_script
  CMake.Dialogs.QtGUI
  CMake.Documentation.SphinxHTML
  CMake.DeveloperReference.HTML)
  cpack_ifw_configure_file("${CMake_SOURCE_DIR}/Source/QtIFW/${_script}.qs.in"
    "${CMake_BINARY_DIR}/${_script}.qs")
endforeach()

if(NOT DEFINED CPACK_PACKAGE_FILE_NAME)
  # if the CPACK_PACKAGE_FILE_NAME is not defined by the cache
  # default to source package - system, on cygwin system is not
  # needed
  if(CYGWIN)
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}")
  else()
    set(CPACK_PACKAGE_FILE_NAME
      "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
  endif()
endif()

set(CPACK_PACKAGE_CONTACT "cmake@cmake.org")

if(UNIX)
  set(CPACK_STRIP_FILES "${CMAKE_BIN_DIR}/ccmake;${CMAKE_BIN_DIR}/cmake;${CMAKE_BIN_DIR}/cpack;${CMAKE_BIN_DIR}/ctest")
  set(CPACK_SOURCE_STRIP_FILES "")
  set(CPACK_PACKAGE_EXECUTABLES "ccmake" "CMake")
endif()

set(CPACK_WIX_UPGRADE_GUID "8ffd1d72-b7f1-11e2-8ee5-00238bca4991")

if(MSVC AND NOT "$ENV{WIX}" STREQUAL "")
  set(WIX_CUSTOM_ACTION_ENABLED TRUE)
  if(CMAKE_CONFIGURATION_TYPES)
    set(WIX_CUSTOM_ACTION_MULTI_CONFIG TRUE)
  else()
    set(WIX_CUSTOM_ACTION_MULTI_CONFIG FALSE)
  endif()
else()
  set(WIX_CUSTOM_ACTION_ENABLED FALSE)
endif()

# Set the options file that needs to be included inside CMakeCPackOptions.cmake
set(QT_DIALOG_CPACK_OPTIONS_FILE ${CMake_BINARY_DIR}/Source/QtDialog/QtDialogCPack.cmake)
configure_file("${CMake_SOURCE_DIR}/CMakeCPackOptions.cmake.in"
  "${CMake_BINARY_DIR}/CMakeCPackOptions.cmake" @ONLY)
set(CPACK_PROJECT_CONFIG_FILE "${CMake_BINARY_DIR}/CMakeCPackOptions.cmake")

set(CPACK_SOURCE_IGNORE_FILES
  # Files specific to version control.
  "/\\\\.git/"
  "/\\\\.gitattributes$"
  "/\\\\.github/"
  "/\\\\.gitignore$"
  "/\\\\.hooks-config$"

  # Cygwin package build.
  "/\\\\.build/"

  # Temporary files.
  "\\\\.swp$"
  "\\\\.#"
  "/#"
  "~$"
  )

# include CPack model once all variables are set
include(CPack)
