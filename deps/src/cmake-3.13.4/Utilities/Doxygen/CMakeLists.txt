# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

if(NOT CMake_SOURCE_DIR)
  set(CMakeDeveloperReference_STANDALONE 1)
  cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
  get_filename_component(tmp "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
  get_filename_component(CMake_SOURCE_DIR "${tmp}" PATH)
  include(${CMake_SOURCE_DIR}/Modules/CTestUseLaunchers.cmake)
  include(${CMake_SOURCE_DIR}/Source/CMakeVersionCompute.cmake)
  include(${CMake_SOURCE_DIR}/Source/CMakeInstallDestinations.cmake)
  unset(CMAKE_DATA_DIR)
  unset(CMAKE_DATA_DIR CACHE)
  macro(CMake_OPTIONAL_COMPONENT)
    set(COMPONENT "")
  endmacro()
endif()

project(CMakeDeveloperReference NONE)

#
# Build the reference
#

if (CMake_BUILD_DEVELOPER_REFERENCE OR CMakeDeveloperReference_STANDALONE)

  find_package(Doxygen REQUIRED)

  #
  ## Output formats
  #

  option(CMake_BUILD_DEVELOPER_REFERENCE_HTML "Build CMake Developer Reference - HTML format" ON)
  mark_as_advanced(CMake_BUILD_DEVELOPER_REFERENCE_HTML)
  if(CMake_BUILD_DEVELOPER_REFERENCE_HTML)
    set(GENERATE_HTML YES)
  else()
    set(GENERATE_HTML NO)
  endif()

  option(CMake_BUILD_DEVELOPER_REFERENCE_QTHELP "Build CMake Developer Reference - QtHelp format" OFF)
  mark_as_advanced(CMake_BUILD_DEVELOPER_REFERENCE_QTHELP)
  if(CMake_BUILD_DEVELOPER_REFERENCE_QTHELP)
    set(GENERATE_QHP YES)
    find_program(QHELPGENERATOR_EXECUTABLE
      NAMES qhelpgenerator
      DOC "qhelpgenerator tool"
      )
    if(NOT QHELPGENERATOR_EXECUTABLE)
      message(FATAL_ERROR "QHELPGENERATOR_EXECUTABLE (qhelpgenerator) not found!")
    endif()
  else()
    set(GENERATE_QHP NO)
  endif()

  #
  # Configure the script and the doxyfile, then add target
  #

  if(DOXYGEN_DOT_FOUND)
    set(HAVE_DOT YES)
  else()
    set(HAVE_DOT NO)
  endif()

  if(NOT DOT_PATH)
    get_filename_component(DOT_PATH ${DOXYGEN_DOT_EXECUTABLE} PATH)
  endif()

  configure_file(doxyfile.in doxyfile @ONLY)

  add_custom_target(cmake-developer-reference-all
    ${DOXYGEN_EXECUTABLE} doxyfile
    WORKING_DIRECTORY ${CMakeDeveloperReference_BINARY_DIR})

  add_custom_target(cmake-developer-reference ALL DEPENDS cmake-developer-reference-all)

  #
  # Installation
  #

  if(CMake_BUILD_DEVELOPER_REFERENCE_HTML)
    CMake_OPTIONAL_COMPONENT(cmake-developer-reference-html)
    install(DIRECTORY "${CMakeDeveloperReference_BINARY_DIR}/developer-reference/html"
      DESTINATION ${CMAKE_DOC_DIR}/developer-reference
      ${COMPONENT})
  endif()

  if(CMake_BUILD_DEVELOPER_REFERENCE_QTHELP)
    CMake_OPTIONAL_COMPONENT(cmake-developer-reference-qthelp)
    install(FILES "${CMakeDeveloperReference_BINARY_DIR}/developer-reference/CMakeDeveloperReference-${CMake_VERSION_MAJOR}${CMake_VERSION_MINOR}${CMake_VERSION_PATCH}.qch"
      DESTINATION ${CMAKE_DOC_DIR}/developer-reference
      ${COMPONENT})
  endif()

endif ()
