cmake_minimum_required(VERSION 2.8)

project(Qt4Deploy)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install)

find_package(Qt4 REQUIRED QtMain QtCore QtSql)
include(${QT_USE_FILE})

add_executable(testdeploy MACOSX_BUNDLE testdeploy.cpp)
target_link_libraries(testdeploy ${QT_LIBRARIES})
set_target_properties(testdeploy PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}")

if(CMAKE_CONFIGURATION_TYPES AND QT_QTCORE_LIBRARY_RELEASE AND QT_QTCORE_LIBRARY_DEBUG)
  # note: installing debug Qt libraries from a Qt installation configured with
  # -debug-and-release not yet supported (very low priority).
  install(CODE "
  if(\"\${CMAKE_INSTALL_CONFIG_NAME}\" MATCHES \"^([Dd][Ee][Bb][Uu][Gg])$\")
    return()
  endif()
  ")
endif()

# install the Qt4 app with qsqlite plugin
install(CODE "file(REMOVE_RECURSE \"${CMAKE_INSTALL_PREFIX}\")")
install(TARGETS testdeploy DESTINATION .)
include(../../Modules/DeployQt4.cmake)
if(APPLE)
  install_qt4_executable(testdeploy.app "qsqlite")
elseif(WIN32)
  install_qt4_executable(testdeploy.exe "qsqlite")
else()
  install_qt4_executable(testdeploy "qsqlite")
endif()


# test depends on standard qsqlite plugin
if(QT_QSQLITE_PLUGIN_DEBUG OR QT_QSQLITE_PLUGIN_RELEASE)

  # test the deployed Qt application
  if(APPLE)
    install(CODE "
      message(STATUS \"executing: ${CMAKE_INSTALL_PREFIX}/testdeploy.app/Contents/MacOS/testdeploy\")
      execute_process(COMMAND \"${CMAKE_INSTALL_PREFIX}/testdeploy.app/Contents/MacOS/testdeploy\"
                      RESULT_VARIABLE result)
      if(NOT result STREQUAL \"0\")
        message(FATAL_ERROR \"error running testdeploy app\")
      endif()
    ")
  else()
    install(CODE "
      message(STATUS \"executing: ${CMAKE_INSTALL_PREFIX}/testdeploy\")
      execute_process(COMMAND \"${CMAKE_INSTALL_PREFIX}/testdeploy\"
                      RESULT_VARIABLE result)
      if(NOT result STREQUAL \"0\")
        message(FATAL_ERROR \"error running testdeploy app\")
      endif()
    ")
  endif()

  # custom target to install and test the installation at build time
  if(CMAKE_CONFIGURATION_TYPES)
    set(install_config "-DCMAKE_INSTALL_CONFIG_NAME=${CMAKE_CFG_INTDIR}")
  endif()

  add_custom_target(testdeploy_test ALL
    COMMAND ${CMAKE_COMMAND} ${install_config} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake
    COMMENT "${CMAKE_COMMAND} ${install_config} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake"
    DEPENDS testdeploy)

endif()
