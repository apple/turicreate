# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindLibRHash
------------

Find LibRHash include directory and library.

Imported Targets
^^^^^^^^^^^^^^^^

An :ref:`imported target <Imported targets>` named
``LibRHash::LibRHash`` is provided if LibRHash has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``LibRHash_FOUND``
  True if LibRHash was found, false otherwise.
``LibRHash_INCLUDE_DIRS``
  Include directories needed to include LibRHash headers.
``LibRHash_LIBRARIES``
  Libraries needed to link to LibRHash.

Cache Variables
^^^^^^^^^^^^^^^

This module uses the following cache variables:

``LibRHash_LIBRARY``
  The location of the LibRHash library file.
``LibRHash_INCLUDE_DIR``
  The location of the LibRHash include directory containing ``rhash.h``.

The cache variables should not be used by project code.
They may be set by end users to point at LibRHash components.
#]=======================================================================]

#-----------------------------------------------------------------------------
find_library(LibRHash_LIBRARY
  NAMES rhash
  )
mark_as_advanced(LibRHash_LIBRARY)

find_path(LibRHash_INCLUDE_DIR
  NAMES rhash.h
  )
mark_as_advanced(LibRHash_INCLUDE_DIR)

#-----------------------------------------------------------------------------
include(${CMAKE_CURRENT_LIST_DIR}/../../Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibRHash
  FOUND_VAR LibRHash_FOUND
  REQUIRED_VARS LibRHash_LIBRARY LibRHash_INCLUDE_DIR
  )
set(LIBRHASH_FOUND ${LibRHash_FOUND})

#-----------------------------------------------------------------------------
# Provide documented result variables and targets.
if(LibRHash_FOUND)
  set(LibRHash_INCLUDE_DIRS ${LibRHash_INCLUDE_DIR})
  set(LibRHash_LIBRARIES ${LibRHash_LIBRARY})
  if(NOT TARGET LibRHash::LibRHash)
    add_library(LibRHash::LibRHash UNKNOWN IMPORTED)
    set_target_properties(LibRHash::LibRHash PROPERTIES
      IMPORTED_LOCATION "${LibRHash_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LibRHash_INCLUDE_DIRS}"
      )
  endif()
endif()
