project(foo Java)

cmake_minimum_required (VERSION 3.5)
set(CMAKE_VERBOSE_MAKEFILE 1)

find_package(Java COMPONENTS Development)
include(UseJava)

add_jar(${PROJECT_NAME} Foo.java)
export_jars(
  TARGETS ${PROJECT_NAME}
  NAMESPACE foo::
  FILE JavaBuildExportTestConfig.cmake)
