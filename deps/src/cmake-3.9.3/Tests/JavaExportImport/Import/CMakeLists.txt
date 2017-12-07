project(import Java)

cmake_minimum_required (VERSION 3.5)
set(CMAKE_VERBOSE_MAKEFILE 1)

find_package(Java COMPONENTS Development)
include(UseJava)

find_package(JavaBuildExportTest REQUIRED)
find_package(JavaInstallExportTest REQUIRED)

add_jar(${PROJECT_NAME}
  SOURCES Import.java
  INCLUDE_JARS foo::foo bar::bar)
