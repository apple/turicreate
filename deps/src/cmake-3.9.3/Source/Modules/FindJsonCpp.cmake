# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindJsonCpp
-----------

Find JsonCpp includes and library.

Imported Targets
^^^^^^^^^^^^^^^^

An :ref:`imported target <Imported targets>` named
``JsonCpp::JsonCpp`` is provided if JsonCpp has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``JsonCpp_FOUND``
  True if JsonCpp was found, false otherwise.
``JsonCpp_INCLUDE_DIRS``
  Include directories needed to include JsonCpp headers.
``JsonCpp_LIBRARIES``
  Libraries needed to link to JsonCpp.
``JsonCpp_VERSION_STRING``
  The version of JsonCpp found.
  May not be set for JsonCpp versions prior to 1.0.
``JsonCpp_VERSION_MAJOR``
  The major version of JsonCpp.
``JsonCpp_VERSION_MINOR``
  The minor version of JsonCpp.
``JsonCpp_VERSION_PATCH``
  The patch version of JsonCpp.

Cache Variables
^^^^^^^^^^^^^^^

This module uses the following cache variables:

``JsonCpp_LIBRARY``
  The location of the JsonCpp library file.
``JsonCpp_INCLUDE_DIR``
  The location of the JsonCpp include directory containing ``json/json.h``.

The cache variables should not be used by project code.
They may be set by end users to point at JsonCpp components.
#]=======================================================================]

#-----------------------------------------------------------------------------
find_library(JsonCpp_LIBRARY
  NAMES jsoncpp
  )
mark_as_advanced(JsonCpp_LIBRARY)

find_path(JsonCpp_INCLUDE_DIR
  NAMES json/json.h
  PATH_SUFFIXES jsoncpp
  )
mark_as_advanced(JsonCpp_INCLUDE_DIR)

#-----------------------------------------------------------------------------
# Extract version number if possible.
set(_JsonCpp_H_REGEX "^#[ \t]*define[ \t]+JSONCPP_VERSION_STRING[ \t]+\"(([0-9]+)\\.([0-9]+)\\.([0-9]+)[^\"]*)\".*$")
if(JsonCpp_INCLUDE_DIR AND EXISTS "${JsonCpp_INCLUDE_DIR}/json/version.h")
  file(STRINGS "${JsonCpp_INCLUDE_DIR}/json/version.h" _JsonCpp_H REGEX "${_JsonCpp_H_REGEX}")
else()
  set(_JsonCpp_H "")
endif()
if(_JsonCpp_H MATCHES "${_JsonCpp_H_REGEX}")
  set(JsonCpp_VERSION_STRING "${CMAKE_MATCH_1}")
  set(JsonCpp_VERSION_MAJOR "${CMAKE_MATCH_2}")
  set(JsonCpp_VERSION_MINOR "${CMAKE_MATCH_3}")
  set(JsonCpp_VERSION_PATCH "${CMAKE_MATCH_4}")
else()
  set(JsonCpp_VERSION_STRING "")
  set(JsonCpp_VERSION_MAJOR "")
  set(JsonCpp_VERSION_MINOR "")
  set(JsonCpp_VERSION_PATCH "")
endif()
unset(_JsonCpp_H_REGEX)
unset(_JsonCpp_H)

#-----------------------------------------------------------------------------
include(${CMAKE_CURRENT_LIST_DIR}/../../Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JsonCpp
  FOUND_VAR JsonCpp_FOUND
  REQUIRED_VARS JsonCpp_LIBRARY JsonCpp_INCLUDE_DIR
  VERSION_VAR JsonCpp_VERSION_STRING
  )
set(JSONCPP_FOUND ${JsonCpp_FOUND})

#-----------------------------------------------------------------------------
# Provide documented result variables and targets.
if(JsonCpp_FOUND)
  set(JsonCpp_INCLUDE_DIRS ${JsonCpp_INCLUDE_DIR})
  set(JsonCpp_LIBRARIES ${JsonCpp_LIBRARY})
  if(NOT TARGET JsonCpp::JsonCpp)
    add_library(JsonCpp::JsonCpp UNKNOWN IMPORTED)
    set_target_properties(JsonCpp::JsonCpp PROPERTIES
      IMPORTED_LOCATION "${JsonCpp_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${JsonCpp_INCLUDE_DIRS}"
      IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
      )
  endif()
endif()
