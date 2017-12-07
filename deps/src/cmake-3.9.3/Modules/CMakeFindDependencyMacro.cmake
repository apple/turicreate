# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# CMakeFindDependencyMacro
# -------------------------
#
# ::
#
#     find_dependency(<dep> [...])
#
#
# ``find_dependency()`` wraps a :command:`find_package` call for a package
# dependency. It is designed to be used in a <package>Config.cmake file, and it
# forwards the correct parameters for QUIET and REQUIRED which were passed to
# the original :command:`find_package` call.  It also sets an informative
# diagnostic message if the dependency could not be found.
#
# Any additional arguments specified are forwarded to :command:`find_package`.
#

macro(find_dependency dep)
  if (NOT ${dep}_FOUND)
    set(cmake_fd_quiet_arg)
    if(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
      set(cmake_fd_quiet_arg QUIET)
    endif()
    set(cmake_fd_required_arg)
    if(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
      set(cmake_fd_required_arg REQUIRED)
    endif()

    get_property(cmake_fd_alreadyTransitive GLOBAL PROPERTY
      _CMAKE_${dep}_TRANSITIVE_DEPENDENCY
    )

    find_package(${dep} ${ARGN}
      ${cmake_fd_quiet_arg}
      ${cmake_fd_required_arg}
    )

    if(NOT DEFINED cmake_fd_alreadyTransitive OR cmake_fd_alreadyTransitive)
      set_property(GLOBAL PROPERTY _CMAKE_${dep}_TRANSITIVE_DEPENDENCY TRUE)
    endif()

    if (NOT ${dep}_FOUND)
      set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE "${CMAKE_FIND_PACKAGE_NAME} could not be found because dependency ${dep} could not be found.")
      set(${CMAKE_FIND_PACKAGE_NAME}_FOUND False)
      return()
    endif()
    set(cmake_fd_required_arg)
    set(cmake_fd_quiet_arg)
    set(cmake_fd_exact_arg)
  endif()
endmacro()
