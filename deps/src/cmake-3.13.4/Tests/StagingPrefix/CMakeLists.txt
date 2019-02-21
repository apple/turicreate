
cmake_minimum_required(VERSION 3.9)
project(StagingPrefix)

# Wipe out the install tree
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/CleanupProject
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/ConsumerBuild
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/ProducerBuild
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/stage
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/prefix
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/ignored
  )
add_custom_target(CleanupTarget ALL DEPENDS ${CMAKE_BINARY_DIR}/CleanupProject)
set_property(
  SOURCE ${CMAKE_BINARY_DIR}/CleanupProject
  PROPERTY SYMBOLIC 1
  )

get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_isMultiConfig)
  set(NESTED_CONFIG_TYPE -C "${CMAKE_CFG_INTDIR}")
else()
  if(CMAKE_BUILD_TYPE)
    set(NESTED_CONFIG_TYPE -C "${CMAKE_BUILD_TYPE}")
  else()
    set(NESTED_CONFIG_TYPE)
  endif()
endif()

# Build and install the producer.
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/ProducerProject
  COMMAND ${CMAKE_CTEST_COMMAND} ${NESTED_CONFIG_TYPE}
    --build-and-test
    ${CMAKE_SOURCE_DIR}/Producer
    ${CMAKE_BINARY_DIR}/ProducerBuild
    --build-noclean
    --build-project Producer
    --build-target install
    --build-generator ${CMAKE_GENERATOR}
    --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
    --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
    --build-options
      -DCMAKE_VERBOSE_MAKEFILE=1
      "-DCMAKE_STAGING_PREFIX=${CMAKE_BINARY_DIR}/stage"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/prefix"
  VERBATIM
  )

add_custom_target(ProducerTarget ALL DEPENDS ${CMAKE_BINARY_DIR}/ProducerProject)
add_dependencies(ProducerTarget CleanupTarget)
set_property(
  SOURCE ${CMAKE_BINARY_DIR}/ProducerProject
  PROPERTY SYMBOLIC 1
  )

if(NOT WIN32)
  file(WRITE
    "${CMAKE_BINARY_DIR}/ignored/${CMAKE_BINARY_DIR}/stage/include/ignored.h"
    "#define IGNORED\n"
  )
endif()

# Build and install the consumer.
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/ConsumerProject
  COMMAND ${CMAKE_CTEST_COMMAND} ${NESTED_CONFIG_TYPE}
   --build-and-test
   ${CMAKE_SOURCE_DIR}/Consumer
   ${CMAKE_BINARY_DIR}/ConsumerBuild
   --build-noclean
   --build-project Consumer
   --build-target install
   --build-generator ${CMAKE_GENERATOR}
   --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
   --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
   --build-options
      "-DCMAKE_FIND_ROOT_PATH=${CMAKE_BINARY_DIR}/ignored"
      "-DCMAKE_STAGING_PREFIX=${CMAKE_BINARY_DIR}/stage"
      -DCMAKE_VERBOSE_MAKEFILE=1
  VERBATIM
  )
add_custom_target(ConsumerTarget ALL DEPENDS ${CMAKE_BINARY_DIR}/ConsumerProject)
add_dependencies(ConsumerTarget ProducerTarget)
set_property(
  SOURCE ${CMAKE_BINARY_DIR}/ConsumerProject
  PROPERTY SYMBOLIC 1
  )

add_executable(StagingPrefix main.cpp)
add_dependencies(StagingPrefix ConsumerTarget)
