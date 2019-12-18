# This is an external function
# Usage:
#    make_binary(NAME target
#                SOURCES a.cpp b.cpp
#                REQUIRES libx liby
#                MAC_REQUIRES libz libzz
#                LINUX_REQUIRES libk libj)
# Example:
#
# make_binary(NAME wscmd
#              SOURCES
#                wscmd.cpp
#              REQUIRES
#                fileio
#             )
#
# This generates a binary with the provided target name.
#
# NAME and SOURCES must be specified.
# REQUIRES lists all dependent libraries. These can be:
#   - other libraries built by the the turicreate build system
#   - builtin libraries
#   - system libraries
# MAC_REQUIRES lists all dependent libraries which are included only on Mac.
# LINUX_REQUIRES lists all dependent libraries which are included only on Linux.
# OUTPUT_NAME is the final output name of the target. Defaults to the target name
# if not specified
#
# All other targets which depends on this library (using the "requires" function)
# will automatically include all recursive dependencies.
#
# Boost, pthread is always added as a default dependency.
# when possible.
function (make_executable NAME)
  set(one_value_args COMPILE_FLAGS OUTPUT_NAME)
  set(multi_value_args
    SOURCES REQUIRES MAC_REQUIRES LINUX_REQUIRES
    COMPILE_FLAGS_EXTRA COMPILE_FLAGS_EXTRA_CLANG COMPILE_FLAGS_EXTRA_GCC)
  CMAKE_PARSE_ARGUMENTS(make_library "" "${one_value_args}" "${multi_value_args}" ${ARGN})
  if(NOT make_library_SOURCES)
    MESSAGE(FATAL_ERROR "make_executable call with no sources")
  endif()

  if (APPLE)
    if (make_library_MAC_REQUIRES)
      set(make_library_REQUIRES ${make_library_REQUIRES} ${make_library_MAC_REQUIRES})
    endif()
  else()
    if (make_library_LINUX_REQUIRES)
      set(make_library_REQUIRES ${make_library_REQUIRES} ${make_library_LINUX_REQUIRES})
    endif()
  endif()

  add_executable(${NAME} ${make_library_SOURCES})

  if (make_library_COMPILE_FLAGS_EXTRA)
    target_compile_options(${NAME} PRIVATE ${make_library_COMPILE_FLAGS_EXTRA})
  endif()

  if (CLANG)
    if (make_library_COMPILE_FLAGS_EXTRA_CLANG)
      target_compile_options(${NAME} PRIVATE ${make_library_COMPILE_FLAGS_EXTRA_CLANG})
    endif()
  endif()

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (make_library_COMPILE_FLAGS_EXTRA_GCC)
      target_compile_options(${NAME} PRIVATE ${make_library_COMPILE_FLAGS_EXTRA_GCC})
    endif()
  endif()

  make_target_impl("${NAME}" "${make_library_COMPILE_FLAGS}"
    "${make_library_REQUIRES}" FALSE FALSE FALSE FALSE)
  if (make_library_OUTPUT_NAME)
          message(STATUS "make_executable ${NAME} ===> ${make_library_OUTPUT_NAME}")
          set_target_properties(${NAME} PROPERTIES OUTPUT_NAME ${make_library_OUTPUT_NAME})
  endif()
  # this is really annoying
  # There really isn't a clean way to this, but on Mac Anaconda's libpython2.7.dylib
  # has it's install name set to just libpython2.7.dylib and not @rapth/libpython2.7.dylib
  # We need to patch this.
  if (APPLE)
          add_custom_command(TARGET ${NAME} POST_BUILD
                  COMMAND install_name_tool $<TARGET_FILE:${NAME}> -change libpython2.7.dylib @rpath/libpython2.7.dylib)
  endif()

  if(NOT CLANG)
    if (NOT WIN32)
      # set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS "-static-libstdc++")
    endif()
  endif()
endfunction()



