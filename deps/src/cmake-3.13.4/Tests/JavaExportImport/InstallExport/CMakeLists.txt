project(bar Java)

cmake_minimum_required (VERSION 3.5)
set(CMAKE_VERBOSE_MAKEFILE 1)

find_package(Java COMPONENTS Development)
include(UseJava)

add_jar(${PROJECT_NAME} Bar.java)
install_jar(${PROJECT_NAME} DESTINATION share/java)
install_jar_exports(
  TARGETS ${PROJECT_NAME}
  NAMESPACE bar::
  FILE JavaInstallExportTestConfig.cmake
  DESTINATION share/cmake)
