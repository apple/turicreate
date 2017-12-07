cmake_minimum_required (VERSION 3.5)
project(JavaExportImport)
if(NOT DEFINED CMake_TEST_NESTED_MAKE_PROGRAM AND NOT CMAKE_GENERATOR MATCHES "Visual Studio")
  set(CMake_TEST_NESTED_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM}")
endif()

find_package(Java COMPONENTS Development)

# Wipe out the install tree to make sure the exporter works.
add_custom_command(
  OUTPUT ${JavaExportImport_BINARY_DIR}/CleanupProject
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${JavaExportImport_BINARY_DIR}/Root
  )
add_custom_target(CleanupTarget ALL DEPENDS ${JavaExportImport_BINARY_DIR}/CleanupProject)
set_property(
  SOURCE ${JavaExportImport_BINARY_DIR}/CleanupProject
  PROPERTY SYMBOLIC 1
  )

if(CMAKE_CONFIGURATION_TYPES)
  set(NESTED_CONFIG_TYPE -C "${CMAKE_CFG_INTDIR}")
else()
  if(CMAKE_BUILD_TYPE)
    set(NESTED_CONFIG_TYPE -C "${CMAKE_BUILD_TYPE}")
  else()
    set(NESTED_CONFIG_TYPE)
  endif()
endif()

configure_file(${JavaExportImport_SOURCE_DIR}/InitialCache.cmake.in
               ${JavaExportImport_BINARY_DIR}/InitialCache.cmake @ONLY)

# Build the build exporter.
add_custom_command(
  OUTPUT ${JavaExportImport_BINARY_DIR}/BuildExportProject
  COMMAND ${CMAKE_CTEST_COMMAND} ${NESTED_CONFIG_TYPE}
    --build-and-test
    ${JavaExportImport_SOURCE_DIR}/BuildExport
    ${JavaExportImport_BINARY_DIR}/BuildExport
    --build-noclean
    --build-project BuildExport
    --build-generator ${CMAKE_GENERATOR}
    --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
    --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
    --build-options -C${JavaExportImport_BINARY_DIR}/InitialCache.cmake
  VERBATIM
  )
add_custom_target(BuildExportTarget ALL DEPENDS ${JavaExportImport_BINARY_DIR}/BuildExportProject)
add_dependencies(BuildExportTarget CleanupTarget)
set_property(
  SOURCE ${JavaExportImport_BINARY_DIR}/BuildExportProject
  PROPERTY SYMBOLIC 1
  )

# Build and install the install exporter.
add_custom_command(
  OUTPUT ${JavaExportImport_BINARY_DIR}/InstallExportProject
  COMMAND ${CMAKE_CTEST_COMMAND} ${NESTED_CONFIG_TYPE}
    --build-and-test
    ${JavaExportImport_SOURCE_DIR}/InstallExport
    ${JavaExportImport_BINARY_DIR}/InstallExport
    --build-noclean
    --build-project InstallExport
    --build-target install
    --build-generator ${CMAKE_GENERATOR}
    --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
    --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
    --build-options -C${JavaExportImport_BINARY_DIR}/InitialCache.cmake
  VERBATIM
  )
add_custom_target(InstallExportTarget ALL DEPENDS ${JavaExportImport_BINARY_DIR}/InstallExportProject)
add_dependencies(InstallExportTarget CleanupTarget)
set_property(
  SOURCE ${JavaExportImport_BINARY_DIR}/InstallExportProject
  PROPERTY SYMBOLIC 1
  )

# Build and install the importer.
add_custom_command(
  OUTPUT ${JavaExportImport_BINARY_DIR}/ImportProject
  COMMAND ${CMAKE_CTEST_COMMAND} ${NESTED_CONFIG_TYPE}
    --build-and-test
    ${JavaExportImport_SOURCE_DIR}/Import
    ${JavaExportImport_BINARY_DIR}/Import
    --build-noclean
    --build-project Import
    --build-generator ${CMAKE_GENERATOR}
    --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
    --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
    --build-options
      -C${JavaExportImport_BINARY_DIR}/InitialCache.cmake
      -DJavaBuildExportTest_DIR:PATH=${JavaExportImport_BINARY_DIR}/BuildExport
      -DJavaInstallExportTest_DIR:PATH=${JavaExportImport_BINARY_DIR}/Root/share/cmake
  VERBATIM
  )
add_custom_target(ImportTarget ALL DEPENDS ${JavaExportImport_BINARY_DIR}/ImportProject)
add_dependencies(ImportTarget BuildExportTarget InstallExportTarget)
set_property(
  SOURCE ${JavaExportImport_BINARY_DIR}/ImportProject
  PROPERTY SYMBOLIC 1
  )

add_executable(JavaExportImport main.c)
add_dependencies(JavaExportImport ImportTarget)
