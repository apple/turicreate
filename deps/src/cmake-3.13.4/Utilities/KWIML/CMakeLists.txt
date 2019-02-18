#
# Copyright Kitware, Inc.
# Distributed under the OSI-approved BSD 3-Clause License.
# See accompanying file Copyright.txt for details.
#
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
  set(kwiml_standalone 1)
  project(KWIML)
  include(CTest)
  mark_as_advanced(BUILD_TESTING)
  if(BUILD_TESTING)
    set(KWIML_TEST_ENABLE 1)
  endif()
  if(NOT DEFINED KWIML_INSTALL_INCLUDE_DIR)
    set(KWIML_INSTALL_INCLUDE_DIR include)
  endif()
  set(KWIML_INCLUDE_PREFIX kwiml)
else()
  cmake_minimum_required(VERSION 2.8.2 FATAL_ERROR)
  set(kwiml_standalone 0)
  if(KWIML_INSTALL_INCLUDE_DIR AND NOT DEFINED KWIML_INCLUDE_PREFIX)
    message(FATAL_ERROR "Host project must set KWIML_INCLUDE_PREFIX")
  endif()
endif()

get_property(KWIML_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
foreach(lang ${KWIML_LANGUAGES})
  set(KWIML_LANGUAGE_${lang} 1)
endforeach()
if(NOT KWIML_LANGUAGE_C AND NOT KWIML_LANGUAGE_CXX)
  set(BUILD_TESTING OFF)
endif()

if(KWIML_INSTALL_INCLUDE_DIR)
  install(FILES
    include/kwiml/abi.h
    include/kwiml/int.h
    DESTINATION ${KWIML_INSTALL_INCLUDE_DIR}/${KWIML_INCLUDE_PREFIX}
    ${KWIML_INSTALL_INCLUDE_OPTIONS}
    )
endif()

if(KWIML_TEST_ENABLE)
  add_subdirectory(test)
endif()

if(NOT kwiml_standalone)
  return()
endif()

#----------------------------------------------------------------------------
set(KWIML_VERSION 1.0.0)
if(KWIML_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
  set(KWIML_VERSION_MAJOR "${CMAKE_MATCH_1}")
  set(KWIML_VERSION_MINOR "${CMAKE_MATCH_2}")
  set(KWIML_VERSION_PATCH "${CMAKE_MATCH_3}")
  math(EXPR KWIML_VERSION_DECIMAL
    "${KWIML_VERSION_MAJOR}*1000000 + ${KWIML_VERSION_MINOR}*1000 + ${KWIML_VERSION_PATCH}")
else()
  message(FATAL_ERROR "Failed to parse KWIML_VERSION='${KWIML_VERSION}'")
endif()

configure_file(src/version.h.in include/kwiml/version.h @ONLY)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/include/kwiml/version.h
  DESTINATION ${KWIML_INSTALL_INCLUDE_DIR}/kwiml
  )

if(NOT KWIML_INSTALL_PACKAGE_DIR)
  set(KWIML_INSTALL_PACKAGE_DIR share/cmake/kwiml-${KWIML_VERSION_MAJOR}.${KWIML_VERSION_MINOR})
endif()

add_library(kwiml INTERFACE)
target_include_directories(kwiml INTERFACE
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${KWIML_INSTALL_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )
export(TARGETS kwiml
  NAMESPACE kwiml::
  FILE kwiml-targets.cmake
  )
install(TARGETS kwiml
  DESTINATION lib
  EXPORT kwiml-targets
  )
install(EXPORT kwiml-targets
  NAMESPACE kwiml::
  DESTINATION ${KWIML_INSTALL_PACKAGE_DIR}
  )

configure_file(src/kwiml-config.cmake.in kwiml-config.cmake @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/kwiml-config-version.cmake"
  VERSION ${KWIML_VERSION}
  COMPATIBILITY AnyNewerVersion
  )
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/kwiml-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/kwiml-config-version.cmake
  DESTINATION ${KWIML_INSTALL_PACKAGE_DIR}
  )
