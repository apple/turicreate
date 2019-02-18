# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

project(QtDialog)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW) # Drop when CMake >= 2.8.11 required
endif()
CMake_OPTIONAL_COMPONENT(cmake-gui)
find_package(Qt5Widgets QUIET)
if (Qt5Widgets_FOUND)
  include_directories(${Qt5Widgets_INCLUDE_DIRS})
  add_definitions(${Qt5Widgets_DEFINITONS})
  macro(qt4_wrap_ui)
    qt5_wrap_ui(${ARGN})
  endmacro()
  macro(qt4_wrap_cpp)
    qt5_wrap_cpp(${ARGN})
  endmacro()
  macro(qt4_add_resources)
    qt5_add_resources(${ARGN})
  endmacro()
  set(CMake_QT_LIBRARIES ${Qt5Widgets_LIBRARIES})
  set(QT_QTMAIN_LIBRARY ${Qt5Core_QTMAIN_LIBRARIES})

  # Remove this when the minimum version of Qt is 4.6.
  add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

  if(CMake_QT_STATIC_QXcbIntegrationPlugin_LIBRARIES)
    list(APPEND CMake_QT_LIBRARIES ${CMake_QT_STATIC_QXcbIntegrationPlugin_LIBRARIES})
    set_property(SOURCE CMakeSetup.cxx
      PROPERTY COMPILE_DEFINITIONS USE_QXcbIntegrationPlugin)
  endif()

  if(CMake_QT_STATIC_QWindowsIntegrationPlugin_LIBRARIES)
    list(APPEND CMake_QT_LIBRARIES ${CMake_QT_STATIC_QWindowsIntegrationPlugin_LIBRARIES})
    set_property(SOURCE CMakeSetup.cxx
      PROPERTY COMPILE_DEFINITIONS USE_QWindowsIntegrationPlugin)
  endif()

  # We need to install platform plugin and add qt.conf for Qt5 on Mac and Windows.
  # FIXME: This should be part of Qt5 CMake scripts, but unfortunately
  # Qt5 support is missing there.
  if(CMake_INSTALL_DEPENDENCIES AND (APPLE OR WIN32))
    macro(install_qt5_plugin _qt_plugin_name _qt_plugins_var)
      get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
      if(EXISTS "${_qt_plugin_path}")
        get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
        get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
        get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
        if(APPLE)
          set(_qt_plugin_dir "PlugIns")
        elseif(WIN32)
          set(_qt_plugin_dir "plugins")
        endif()
        set(_qt_plugin_dest "${_qt_plugin_dir}/${_qt_plugin_type}")
        install(FILES "${_qt_plugin_path}"
          DESTINATION "${_qt_plugin_dest}"
          ${COMPONENT})
        set(${_qt_plugins_var}
          "${${_qt_plugins_var}};\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${_qt_plugin_dest}/${_qt_plugin_file}")
      else()
        message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
      endif()
    endmacro()
    if(APPLE)
      install_qt5_plugin("Qt5::QCocoaIntegrationPlugin" QT_PLUGINS)
      file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        "[Paths]\nPlugins = ${_qt_plugin_dir}\n")
      install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/Resources"
        ${COMPONENT})
    elseif(WIN32 AND NOT CMake_QT_STATIC_QWindowsIntegrationPlugin_LIBRARIES)
      install_qt5_plugin("Qt5::QWindowsIntegrationPlugin" QT_PLUGINS)
      file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        "[Paths]\nPlugins = ../${_qt_plugin_dir}\n")
      install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        DESTINATION bin
        ${COMPONENT})
    endif()
  endif()

  if(TARGET Qt5::Core)
    get_property(_Qt5_Core_LOCATION TARGET Qt5::Core PROPERTY LOCATION)
    get_filename_component(Qt_BIN_DIR "${_Qt5_Core_LOCATION}" PATH)
    if(APPLE)
      get_filename_component(Qt_BIN_DIR "${Qt_BIN_DIR}" PATH)
    endif()
  endif()
else()
  set(QT_MIN_VERSION "4.4.0")
  find_package(Qt4 REQUIRED)
  if(NOT QT4_FOUND)
    message(SEND_ERROR "Failed to find Qt 4.4 or greater.")
    return()
  endif()

  include(${QT_USE_FILE})

  set(CMake_QT_LIBRARIES ${QT_LIBRARIES})

endif()

set(SRCS
  AddCacheEntry.cxx
  AddCacheEntry.h
  CMakeSetup.cxx
  CMakeSetupDialog.cxx
  CMakeSetupDialog.h
  FirstConfigure.cxx
  FirstConfigure.h
  QCMake.cxx
  QCMake.h
  QCMakeCacheView.cxx
  QCMakeCacheView.h
  QCMakeWidgets.cxx
  QCMakeWidgets.h
  RegexExplorer.cxx
  RegexExplorer.h
  WarningMessagesDialog.cxx
  WarningMessagesDialog.h
  )
QT4_WRAP_UI(UI_SRCS
  CMakeSetupDialog.ui
  Compilers.ui
  CrossCompiler.ui
  AddCacheEntry.ui
  RegexExplorer.ui
  WarningMessagesDialog.ui
  )
QT4_WRAP_CPP(MOC_SRCS
  AddCacheEntry.h
  Compilers.h
  CMakeSetupDialog.h
  FirstConfigure.h
  QCMake.h
  QCMakeCacheView.h
  QCMakeWidgets.h
  RegexExplorer.h
  WarningMessagesDialog.h
  )
QT4_ADD_RESOURCES(RC_SRCS CMakeSetup.qrc)

set(SRCS ${SRCS} ${UI_SRCS} ${MOC_SRCS} ${RC_SRCS})
if(WIN32)
  set(SRCS ${SRCS} CMakeSetup.rc)
endif()
if(APPLE)
  set(SRCS ${SRCS} CMakeSetup.icns)
  set(MACOSX_BUNDLE_ICON_FILE CMakeSetup.icns)
  set_source_files_properties(CMakeSetup.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
endif()

if(USE_LGPL)
  install(FILES ${CMake_SOURCE_DIR}/Licenses/LGPLv${USE_LGPL}.txt
    DESTINATION ${CMAKE_DATA_DIR}/Licenses
    ${COMPONENT})
  set_property(SOURCE CMakeSetupDialog.cxx
    PROPERTY COMPILE_DEFINITIONS USE_LGPL="${USE_LGPL}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(cmake-gui WIN32 MACOSX_BUNDLE ${SRCS} ${MANIFEST_FILE})
target_link_libraries(cmake-gui CMakeLib ${QT_QTMAIN_LIBRARY} ${CMake_QT_LIBRARIES})

if(WIN32)
  target_sources(cmake-gui PRIVATE $<TARGET_OBJECTS:CMakeVersion>)
endif()

# cmake-gui has not been updated for `include-what-you-use`.
# Block the tool until this is done.
set_target_properties(cmake-gui PROPERTIES
  CXX_INCLUDE_WHAT_YOU_USE ""
  )

# Files generated by MOC, RCC, and UIC may produce clang-tidy warnings.
# We generate a dummy .clang-tidy file in the binary directory that disables
# all clang-tidy checks except one that will never match.  This one check is
# necessary; clang-tidy reports an error when no checks are enabled.
# Since the Qt code generators will generate source files in the binary tree,
# clang-tidy will load the configuration from this dummy file when the sources
# are built.
file(WRITE "${QtDialog_BINARY_DIR}/.clang-tidy" "
---
Checks: '-*,llvm-twine-local'
...
")

if(APPLE)
  file(STRINGS "${CMake_SOURCE_DIR}/Copyright.txt" copyright_line
    LIMIT_COUNT 1 REGEX "^Copyright 2000-20[0-9][0-9] Kitware")

  set_target_properties(cmake-gui PROPERTIES
    OUTPUT_NAME CMake
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_BUNDLE_VERSION}"
    # TBD: MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_BUNDLE_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "${copyright_line}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.cmake.cmake"
    )

  # Create a symlink in the build tree to provide a "cmake-gui" next
  # to the "cmake" executable that refers to the application bundle.
  add_custom_command(TARGET cmake-gui POST_BUILD
    COMMAND ln -sf CMake.app/Contents/MacOS/CMake
                   $<TARGET_FILE_DIR:cmake>/cmake-gui
    )
endif()
set(CMAKE_INSTALL_DESTINATION_ARGS
  BUNDLE DESTINATION "${CMAKE_BUNDLE_LOCATION}" ${COMPONENT})

install(TARGETS cmake-gui
  RUNTIME DESTINATION bin ${COMPONENT}
  ${CMAKE_INSTALL_DESTINATION_ARGS})

if(UNIX AND NOT APPLE)
  foreach (size IN ITEMS 32 128)
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/CMakeSetup${size}.png"
      DESTINATION "${CMAKE_XDGDATA_DIR}/icons/hicolor/${size}x${size}/apps"
      ${COMPONENT}
      RENAME      "CMakeSetup.png")
  endforeach ()

  # install a desktop file so CMake appears in the application start menu
  # with an icon
  install(FILES cmake-gui.desktop
    DESTINATION "${CMAKE_XDGDATA_DIR}/applications"
    ${COMPONENT})
  install(FILES cmakecache.xml
    DESTINATION "${CMAKE_XDGDATA_DIR}/mime/packages"
    ${COMPONENT})
endif()

if(APPLE)
  install(CODE "
    execute_process(COMMAND ln -s \"../MacOS/CMake\" cmake-gui
        WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin)
  " ${COMPONENT})
endif()

if(CMake_INSTALL_DEPENDENCIES AND (APPLE OR WIN32))
  # install rules for including 3rd party libs such as Qt
  # if a system Qt is used (e.g. installed in /usr/lib/), it will not be included in the installation
  set(fixup_exe "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/cmake-gui${CMAKE_EXECUTABLE_SUFFIX}")
  if(APPLE)
    set(fixup_exe "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/MacOS/CMake")
  endif()
  install(CODE "
    include(\"${CMake_SOURCE_DIR}/Modules/BundleUtilities.cmake\")
    set(BU_CHMOD_BUNDLE_ITEMS ON)
    fixup_bundle(\"${fixup_exe}\" \"${QT_PLUGINS}\" \"${Qt_BIN_DIR};${QT_LIBRARY_DIR};${QT_BINARY_DIR}\")
  " ${COMPONENT})
endif()

set(CMAKE_PACKAGE_QTGUI TRUE)
configure_file("${QtDialog_SOURCE_DIR}/QtDialogCPack.cmake.in"
  "${QtDialog_BINARY_DIR}/QtDialogCPack.cmake" @ONLY)
