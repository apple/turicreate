# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

# Scan the Boost headers and determine the library dependencies.  Note
# that this script only scans one Boost version at once; invoke once
# for each Boost release.  Note that this does require the headers for
# a given component to match the library name, since this computes
# inter-library dependencies.  Library components for which this
# assumption does not hold true and which have dependencies on other
# Boost libraries will require special-casing.  It also doesn't handle
# private dependencies not described in the headers, for static
# library dependencies--this is also a limitation of auto-linking, and
# I'm unaware of any specific instances where this would be
# problematic.
#
# Invoke in script mode, defining these variables:
# BOOST_DIR - the root of the boost includes
#
# The script will process each directory under the root as a
# "component".  For each component, all the headers will be scanned to
# determine the components it depends upon by following all the
# possible includes from this component.  This is to match the
# behaviour of autolinking.

# Written by Roger Leigh <rleigh@codelibre.net>

# Determine header dependencies on libraries using the embedded dependency information.
#
# component - the component to check (uses all headers from boost/${component})
# includedir - the path to the Boost headers
# _ret_libs - list of library dependencies
#
function(_Boost_FIND_COMPONENT_DEPENDENCIES component includedir _ret_libs)
  # _boost_unprocessed_headers - list of headers requiring parsing
  # _boost_processed_headers - headers already parsed (or currently being parsed)
  # _boost_new_headers - new headers discovered for future processing

  set(library_component FALSE)

  # Start by finding all headers for the component; header
  # dependencies via #include will be solved by future passes

  # Special-case since it is part of mpi; look only in boost/mpi/python*
  if(component STREQUAL "mpi_python")
    set(_boost_DEPS "python")
    set(library_component TRUE)
    file(GLOB_RECURSE _boost_unprocessed_headers
         RELATIVE "${includedir}"
         "${includedir}/boost/mpi/python/*")
    list(INSERT _boost_unprocessed_headers 0 "${includedir}/boost/mpi/python.hpp")
  # Special-case since it is a serialization variant; look in boost/serialization
  elseif(component STREQUAL "wserialization")
    set(library_component TRUE)
    file(GLOB_RECURSE _boost_unprocessed_headers
         RELATIVE "${includedir}"
         "${includedir}/boost/serialization/*")
    list(INSERT _boost_unprocessed_headers 0 "${includedir}/boost/serialization.hpp")
  # Not really a library in its own right, but treat it as one
  elseif(component STREQUAL "math")
    set(library_component TRUE)
    file(GLOB_RECURSE _boost_unprocessed_headers
         RELATIVE "${includedir}"
         "${includedir}/boost/math/*")
    list(INSERT _boost_unprocessed_headers 0 "${includedir}/boost/math.hpp")
  # Single test header
  elseif(component STREQUAL "unit_test_framework")
    set(library_component TRUE)
    set(_boost_unprocessed_headers "${BOOST_DIR}/test/unit_test.hpp")
  # Single test header
  elseif(component STREQUAL "prg_exec_monitor")
    set(library_component TRUE)
    set(_boost_unprocessed_headers "${BOOST_DIR}/test/prg_exec_monitor.hpp")
  # Single test header
  elseif(component STREQUAL "test_exec_monitor")
    set(library_component TRUE)
    set(_boost_unprocessed_headers "${BOOST_DIR}/test/test_exec_monitor.hpp")
  else()
    # Default behaviour where header directory is the same as the library name.
    file(GLOB_RECURSE _boost_unprocessed_headers
         RELATIVE "${includedir}"
         "${includedir}/boost/${component}/*")
    list(INSERT _boost_unprocessed_headers 0 "${includedir}/boost/${component}.hpp")
  endif()

  while(_boost_unprocessed_headers)
    list(APPEND _boost_processed_headers ${_boost_unprocessed_headers})
    foreach(header ${_boost_unprocessed_headers})
      if(EXISTS "${includedir}/${header}")
        file(STRINGS "${includedir}/${header}" _boost_header_includes REGEX "^#[ \t]*include[ \t]*<boost/[^>][^>]*>")
        # The optional whitespace before "#" is intentional
        # (boost/serialization/config.hpp bug).
        file(STRINGS "${includedir}/${header}" _boost_header_deps REGEX "^[ \t]*#[ \t]*define[ \t][ \t]*BOOST_LIB_NAME[ \t][ \t]*boost_")

        foreach(line ${_boost_header_includes})
          string(REGEX REPLACE "^#[ \t]*include[ \t]*<(boost/[^>][^>]*)>.*" "\\1" _boost_header_match "${line}")
          list(FIND _boost_processed_headers "${_boost_header_match}" _boost_header_processed)
          list(FIND _boost_new_headers "${_boost_header_match}" _boost_header_new)
          if (_boost_header_processed EQUAL -1 AND _boost_header_new EQUAL -1)
            list(APPEND _boost_new_headers ${_boost_header_match})
          endif()
        endforeach()

        foreach(line ${_boost_header_deps})
          string(REGEX REPLACE "^[ \t]*#[ \t]*define[ \t][ \t]*BOOST_LIB_NAME[ \t][ \t]*boost_([^ \t][^ \t]*).*" "\\1" _boost_component_match "${line}")
          list(FIND _boost_DEPS "${_boost_component_match}" _boost_dep_found)
          if(_boost_component_match STREQUAL "bzip2" OR
             _boost_component_match STREQUAL "zlib")
            # These components may or may not be required; not
            # possible to tell without knowing where and when
            # BOOST_BZIP2_BINARY and BOOST_ZLIB_BINARY are defined.
            # If building against an external zlib or bzip2, this is
            # undesirable.
            continue()
          endif()
          if(component STREQUAL "mpi" AND
             (_boost_component_match STREQUAL "mpi_python" OR
              _boost_component_match STREQUAL "python"))
            # Optional python dependency; skip to avoid making it a
            # hard dependency (handle as special-case for mpi_python).
            continue()
          endif()
          if (_boost_dep_found EQUAL -1 AND
              NOT "${_boost_component_match}" STREQUAL "${component}")
            list(APPEND _boost_DEPS "${_boost_component_match}")
          endif()
          if("${_boost_component_match}" STREQUAL "${component}")
            set(library_component TRUE)
          endif()
        endforeach()
      endif()
    endforeach()
    set(_boost_unprocessed_headers ${_boost_new_headers})
    unset(_boost_new_headers)
  endwhile()

  # message(STATUS "Unfiltered dependencies for Boost::${component}: ${_boost_DEPS}")

  if(NOT library_component)
    unset(_boost_DEPS)
  endif()
  set(${_ret_libs} ${_boost_DEPS} PARENT_SCOPE)

  #string(REGEX REPLACE ";" " " _boost_DEPS_STRING "${_boost_DEPS}")
  #if (NOT _boost_DEPS_STRING)
  #  set(_boost_DEPS_STRING "(none)")
  #endif()
  #message(STATUS "Dependencies for Boost::${component}: ${_boost_DEPS_STRING}")
endfunction()


message(STATUS "Scanning ${BOOST_DIR}")

# List of all directories and files
file(GLOB boost_contents RELATIVE "${BOOST_DIR}/boost" "${BOOST_DIR}/boost/*")

# Components as directories
foreach(component ${boost_contents})
  if(IS_DIRECTORY "${BOOST_DIR}/boost/${component}")
    list(APPEND boost_components "${component}")
  endif()
endforeach()

# The following components are not top-level directories, so require
# special-casing:

# Special-case mpi_python, since it's a part of mpi
if(IS_DIRECTORY "${BOOST_DIR}/boost/mpi" AND
   IS_DIRECTORY "${BOOST_DIR}/boost/python")
 list(APPEND boost_components "mpi_python")
endif()
# Special-case wserialization, which is a variant of serialization
if(IS_DIRECTORY "${BOOST_DIR}/boost/serialization")
 list(APPEND boost_components "wserialization")
endif()
# Special-case math* since there are six libraries, but no actual math
# library component.  Handle specially when scanning above.
#
# Special-case separate test libraries, which are all part of test
if(EXISTS "${BOOST_DIR}/test/unit_test.hpp")
 list(APPEND boost_components "unit_test_framework")
endif()
if(EXISTS "${BOOST_DIR}/test/prg_exec_monitor.hpp")
 list(APPEND boost_components "prg_exec_monitor")
endif()
if(EXISTS "${BOOST_DIR}/test/test_exec_monitor.hpp")
 list(APPEND boost_components "test_exec_monitor")
endif()

if(boost_components)
  list(SORT boost_components)
endif()

# Process each component defined above
foreach(component ${boost_components})
  string(TOUPPER ${component} UPPERCOMPONENT)
  _Boost_FIND_COMPONENT_DEPENDENCIES("${component}" "${BOOST_DIR}"
                                     _Boost_${UPPERCOMPONENT}_LIBRARY_DEPENDENCIES)
endforeach()

# Output results
foreach(component ${boost_components})
  string(TOUPPER ${component} UPPERCOMPONENT)
  if(_Boost_${UPPERCOMPONENT}_LIBRARY_DEPENDENCIES)
    string(REGEX REPLACE ";" " " _boost_DEPS_STRING "${_Boost_${UPPERCOMPONENT}_LIBRARY_DEPENDENCIES}")
    message(STATUS "set(_Boost_${UPPERCOMPONENT}_DEPENDENCIES ${_boost_DEPS_STRING})")
  endif()
endforeach()
