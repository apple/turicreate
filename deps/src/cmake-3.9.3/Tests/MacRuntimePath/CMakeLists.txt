cmake_minimum_required (VERSION 2.8)
project(MacRuntimePath)
if(NOT DEFINED CMake_TEST_NESTED_MAKE_PROGRAM AND NOT CMAKE_GENERATOR MATCHES "Visual Studio")
  set(CMake_TEST_NESTED_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM}")
endif()

# Wipe out the install tree to make sure the exporter works.
add_custom_command(
  OUTPUT ${MacRuntimePath_BINARY_DIR}/CleanupProject
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${MacRuntimePath_BINARY_DIR}/Root
  )
add_custom_target(CleanupTarget ALL DEPENDS ${MacRuntimePath_BINARY_DIR}/CleanupProject)
set_property(
  SOURCE ${MacRuntimePath_BINARY_DIR}/CleanupProject
  PROPERTY SYMBOLIC 1
  )

configure_file(${MacRuntimePath_SOURCE_DIR}/InitialCache.cmake.in
               ${MacRuntimePath_BINARY_DIR}/InitialCache.cmake @ONLY)

if(CMAKE_CONFIGURATION_TYPES)
  set(NESTED_CONFIG_TYPE -C "${CMAKE_CFG_INTDIR}")
else()
  if(CMAKE_BUILD_TYPE)
    set(NESTED_CONFIG_TYPE -C "${CMAKE_BUILD_TYPE}")
  else()
    set(NESTED_CONFIG_TYPE)
  endif()
endif()

# Build and install the exporter.
add_custom_command(
  OUTPUT ${MacRuntimePath_BINARY_DIR}/ExportProject
  COMMAND ${CMAKE_CTEST_COMMAND} ${NESTED_CONFIG_TYPE}
    --build-and-test
    ${MacRuntimePath_SOURCE_DIR}/A
    ${MacRuntimePath_BINARY_DIR}/A
    --build-noclean
    --build-project MacRuntimePath_A
    --build-target install
    --build-generator ${CMAKE_GENERATOR}
    --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
    --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
    --build-options -C${MacRuntimePath_BINARY_DIR}/InitialCache.cmake
  VERBATIM
  )
add_custom_target(ExportTarget ALL DEPENDS ${MacRuntimePath_BINARY_DIR}/ExportProject)
add_dependencies(ExportTarget CleanupTarget)
set_property(
  SOURCE ${MacRuntimePath_BINARY_DIR}/ExportProject
  PROPERTY SYMBOLIC 1
  )

# Build the importer.
add_custom_command(
  OUTPUT ${MacRuntimePath_BINARY_DIR}/ImportProject
  COMMAND ${CMAKE_CTEST_COMMAND} ${NESTED_CONFIG_TYPE}
   --build-and-test
   ${MacRuntimePath_SOURCE_DIR}/B
   ${MacRuntimePath_BINARY_DIR}/B
   --build-noclean
   --build-project MacRuntimePath_B
   --build-generator ${CMAKE_GENERATOR}
   --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
   --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
   --build-options -C${MacRuntimePath_BINARY_DIR}/InitialCache.cmake
  VERBATIM
  )
add_custom_target(ImportTarget ALL DEPENDS ${MacRuntimePath_BINARY_DIR}/ImportProject)
add_dependencies(ImportTarget ExportTarget)
set_property(
  SOURCE ${MacRuntimePath_BINARY_DIR}/ImportProject
  PROPERTY SYMBOLIC 1
  )
