cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0058 OLD)
project(CustomCommandByproducts C)

# Generate a byproduct in a rule that runs in the target consuming it.
add_custom_command(
  OUTPUT timestamp1.txt
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/byproduct1.c.in byproduct1.c
  BYPRODUCTS byproduct1.c
  COMMAND ${CMAKE_COMMAND} -E touch timestamp1.txt
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/byproduct1.c.in
  )

# Generate a byproduct in a rule that runs in a dependency of the consumer.
add_custom_command(
  OUTPUT timestamp2.txt
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/byproduct2.c.in byproduct2.c
  BYPRODUCTS byproduct2.c
  COMMAND ${CMAKE_COMMAND} -E touch timestamp2.txt
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/byproduct2.c.in
  )
add_custom_target(Producer2 DEPENDS timestamp2.txt)

# Generate a byproduct in a custom target.
add_custom_target(Producer3_4
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/byproduct3.c.in byproduct3.c
  BYPRODUCTS byproduct3.c
  )

# Generate a byproduct in a custom target POST_BUILD command.
add_custom_command(
  TARGET Producer3_4 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/byproduct4.c.in byproduct4.c
  BYPRODUCTS byproduct4.c
  )

add_executable(ProducerExe ProducerExe.c)

# Generate a byproduct in an executable POST_BUILD command.
add_custom_command(
  TARGET ProducerExe POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/byproduct5.c.in byproduct5.c
  BYPRODUCTS byproduct5.c
  )

# Generate a byproduct in an executable PRE_LINK command.
add_custom_command(
  TARGET ProducerExe PRE_LINK
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/byproduct6.c.in byproduct6.c
  BYPRODUCTS byproduct6.c
  )

# Generate a byproduct in an executable PRE_BUILD command.
add_custom_command(
  TARGET ProducerExe PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/byproduct7.c.in byproduct7.c
  BYPRODUCTS byproduct7.c
  )

# Generate a byproduct in a custom command that consumes other byproducts.
add_custom_command(OUTPUT timestamp8.txt
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/byproduct8.c.in byproduct8.c
  COMMAND ${CMAKE_COMMAND} -E touch timestamp8.txt
  BYPRODUCTS byproduct8.c
  DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/byproduct2.c
    ${CMAKE_CURRENT_BINARY_DIR}/byproduct3.c
    ${CMAKE_CURRENT_BINARY_DIR}/byproduct4.c
    ${CMAKE_CURRENT_BINARY_DIR}/byproduct5.c
    ${CMAKE_CURRENT_BINARY_DIR}/byproduct6.c
    ${CMAKE_CURRENT_BINARY_DIR}/byproduct7.c
    ${CMAKE_CURRENT_SOURCE_DIR}/byproduct8.c.in
  )

# Generate the library file of an imported target as a byproduct
# of an external project.
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_isMultiConfig)
  set(cfg /${CMAKE_CFG_INTDIR})
else()
  set(cfg)
endif()
set(ExternalLibrary_LIBRARY
  ${CMAKE_CURRENT_BINARY_DIR}/External-build${cfg}/${CMAKE_STATIC_LIBRARY_PREFIX}ExternalLibrary${CMAKE_STATIC_LIBRARY_SUFFIX}
  )
include(ExternalProject)
ExternalProject_Add(ExternalTarget
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/External"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/External-build"
  PREFIX     "${CMAKE_CURRENT_BINARY_DIR}/External-build/root"
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_BYPRODUCTS "${ExternalLibrary_LIBRARY}"
  )
add_library(ExternalLibrary STATIC IMPORTED)
set_property(TARGET ExternalLibrary PROPERTY IMPORTED_LOCATION ${ExternalLibrary_LIBRARY})
add_dependencies(ExternalLibrary ExternalTarget)

# Generate the library file of an imported target as a byproduct
# of an external project. The byproduct uses <BINARY_DIR> that is substituted
# by the real binary path
if(_isMultiConfig)
  set(cfg /${CMAKE_CFG_INTDIR})
else()
  set(cfg)
endif()
include(ExternalProject)
ExternalProject_Add(ExtTargetSubst
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/External"
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_BYPRODUCTS "<BINARY_DIR>${cfg}/${CMAKE_STATIC_LIBRARY_PREFIX}ExternalLibrary${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )
ExternalProject_Get_Property(ExtTargetSubst binary_dir)
add_library(ExternalLibraryWithSubstitution STATIC IMPORTED)
set_property(TARGET ExternalLibraryWithSubstitution PROPERTY IMPORTED_LOCATION
  ${binary_dir}${cfg}/${CMAKE_STATIC_LIBRARY_PREFIX}ExternalLibrary${CMAKE_STATIC_LIBRARY_SUFFIX})
add_dependencies(ExternalLibraryWithSubstitution ExtTargetSubst)

# Add an executable consuming all the byproducts.
add_executable(CustomCommandByproducts
  CustomCommandByproducts.c
  byproduct1.c timestamp1.txt
  byproduct2.c
  byproduct3.c
  byproduct4.c
  byproduct5.c
  byproduct6.c
  byproduct7.c
  byproduct8.c timestamp8.txt
  )
add_dependencies(CustomCommandByproducts Producer2)
add_dependencies(CustomCommandByproducts Producer3_4)
add_dependencies(CustomCommandByproducts ProducerExe)
target_link_libraries(CustomCommandByproducts ExternalLibrary)

if(CMAKE_GENERATOR STREQUAL "Ninja")
  add_custom_target(CheckNinja ALL
    COMMENT "Checking build.ninja"
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/ninja-check.cmake
    )
endif()
