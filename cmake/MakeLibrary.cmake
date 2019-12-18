
# This is an internal function and should not be used
# Usage:
# make_target_impl(target compile_flags sources requirements is_library SHARED)
#
# Example:
# make_target_impl(fileio "-fPIC"
#                   "asyncurl.cpp;sysutils.cpp"
#                   "logger;dl;pthread;z"
#                   TRUE FALSE)
#
# This generates a target library/binary with the given name. The optional
# compile_flags are appended to the target compile flags. "-fPIC" is ALWAYS
# added for libraries. "sources" is a list listing all the library/binary
# source files.  "requirements" is a list listing all the libraries, and
# builtins this target depends on. IS_LIBRARY must be "TRUE" or "FALSE"
#
# if DYNAMIC is true, a dynamic library is built.
#
# Boost, pthread is always added as a default dependency. 
# when possible.
macro(make_target_impl NAME FLAGS REQUIREMENTS IS_LIBRARY SHARED SHARED_ALL_DEFINED OBJECT)
  # create the target
  if (${IS_LIBRARY})
    message(STATUS "Adding Library: ${NAME}")
  else()
    message(STATUS "Adding Executable: ${NAME}")
    # default dependencies
    target_link_libraries(${NAME} PUBLIC boost pthread)
  endif()

  set_property(TARGET ${NAME} PROPERTY IS_LIBRARY ${IS_LIBRARY})

  # add a custom property to the target listing its dependencies
  if(NOT ${FLAGS} STREQUAL "")
    set_property(TARGET ${NAME} APPEND_STRING PROPERTY COMPILE_FLAGS " ${FLAGS}")
  endif()
  if (${IS_LIBRARY})
    if (NOT WIN32)
      #windows is always fPIC
      set_property(TARGET ${NAME} APPEND_STRING PROPERTY COMPILE_FLAGS " -fPIC")
    endif()
    if (APPLE)
      if (${SHARED})
        if (NOT ${SHARED_ALL_DEFINED})
          set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS " -undefined dynamic_lookup")
        endif()
      endif()
    endif()
  endif()

  if (${IS_LIBRARY})
    if(${SHARED})
      target_link_libraries(${NAME} PRIVATE ${REQUIREMENTS})
    elseif(${OBJECT})
      # TODO we can link the requirements from here when target_link_libraries
      # works with OBJECT library targets (requires CMake 3.12)
      # See https://gitlab.kitware.com/cmake/cmake/issues/14778
      # For now, do nothing.
    else()
      target_link_libraries(${NAME} PUBLIC ${REQUIREMENTS})
    endif()
  else()
    target_link_libraries(${NAME} PUBLIC ${REQUIREMENTS})
  endif()
    
  # Ensure dependencies are tracked in order to make sure compilation order matters.
  add_dependencies(${NAME} "${REQUIREMENTS}")
    
  # make sure dependencies are always built first
  add_dependencies(${NAME} "${_TC_EXTERNAL_DEPENDENCIES}")
  add_dependencies(${NAME} external_dependencies)
endmacro()


# This is an external function
# Usage:
#    make_library(NAME target
#                 SOURCES a.cpp b.cpp
#                 REQUIRES libx liby
#                 MAC_REQUIRES libz libzz
#                 LINUX_REQUIRES libk libj
#                 [SHARED] [OUTPUT_NAME xxxx] [SHARED_ALL_DEFINED]
#                 [OBJECT]
#                 )
# Example:
#
# make_library(NAME fileio
#              SOURCES
#                asyncurl.cpp
#                sysutils.cpp
#                wsconn.cpp
#                s3_api.cpp
#                hdfs.cpp
#               REQUIRES
#                 logger dl pthread z curl xml2 openssl
#               MAC_REQUIRES
#                 iconv
#                 )
# This generates a library with the provided target name.
#
# NAME and SOURCES must be specified.
# REQUIRES lists all dependent libraries. These can be:
#   - other libraries built by the the turicreate build system
#   - builtin libraries
#   - system libraries
# MAC_REQUIRES lists all dependent libraries which are included only on Mac.
# LINUX_REQUIRES lists all dependent libraries which are included only on Linux.
# SHARED will build a shared library instead of a static library
# EXTERNAL_VISIBILITY will make the symbols be publicly visible. Default is hidden
# SHARED_ALL_DEFINED will require shared libraries to have all symbols defined
# OBJECT will build an object library instead of a static library
#
# All other targets which depends on this library (using the "requires" function)
# will automatically include all recursive dependencies.
#
# Boost, pthread is always added as a default dependency.
# when possible.
macro(make_library NAME)
  set(options SHARED EXTERNAL_VISIBILITY SHARED_ALL_DEFINED DEAD_STRIP OBJECT)
  set(one_value_args COMPILE_FLAGS OUTPUT_NAME EXPORT_LINUX_MAP_FILE EXPORT_OSX_MAP_FILE)
  set(multi_value_args
    SOURCES REQUIRES MAC_REQUIRES LINUX_REQUIRES
    COMPILE_FLAGS_EXTRA COMPILE_FLAGS_EXTRA_CLANG COMPILE_FLAGS_EXTRA_GCC)
 CMAKE_PARSE_ARGUMENTS(make_library "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  if(NOT make_library_SOURCES)
    MESSAGE(FATAL_ERROR "make_library call with no sources")
  endif()

  if(TC_DISABLE_OBJECT_BUILDS)
    set(make_library_OBJECT 0)
  endif()

  if(TC_LIST_SOURCE_FILES)
    foreach(_src ${make_library_SOURCES})
      if(${_src} MATCHES "<TARGET_OBJECTS:")
        # Do nothing
      elseif(${_src} MATCHES "^${CMAKE_SOURCE_DIR}")
        message(STATUS "[SOURCE:${_src}]")
      else()
        message(STATUS "[SOURCE:${CMAKE_CURRENT_SOURCE_DIR}/${_src}]")
      endif()
    endforeach()
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

  if (${make_library_SHARED})
    add_library(${NAME} SHARED ${make_library_SOURCES})
  elseif(${make_library_OBJECT})
    add_library(${NAME} OBJECT ${make_library_SOURCES})
  else()
    add_library(${NAME} STATIC ${make_library_SOURCES})
  endif()

  make_target_impl("${NAME}" "${make_library_COMPILE_FLAGS}"
    "${make_library_REQUIRES}" TRUE "${make_library_SHARED}" "${make_library_SHARED_ALL_DEFINED}" "${make_library_OBJECT}")

  if (make_library_OUTPUT_NAME)
          message(STATUS "make_library ${NAME} ===> ${make_library_OUTPUT_NAME}")
          set_target_properties(${NAME} PROPERTIES OUTPUT_NAME ${make_library_OUTPUT_NAME})
  endif()

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

  if (${make_library_EXTERNAL_VISIBILITY} OR ${make_library_OBJECT})
    # do nothing
    message(STATUS "External Visibility: " ${NAME})
    target_compile_options(${NAME} PRIVATE "-fvisibility=default")
    target_compile_options(${NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>)
  else()
    target_compile_options(${NAME} PRIVATE "-fvisibility=hidden")
    target_compile_options(${NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>)
  endif()

  if(NOT CLANG)
    if (NOT WIN32)
      # set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS " -static-libstdc++ ")
    endif()
  endif()

  if(APPLE)
  if(make_library_EXPORT_OSX_MAP_FILE)
    set_property(TARGET ${NAME} APPEND PROPERTY LINK_DEPENDS "${make_library_EXPORT_OSX_MAP_FILE}")
    set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-exported_symbols_list,${make_library_EXPORT_OSX_MAP_FILE} ")
  endif()

  if(make_library_DEAD_STRIP)
    set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-dead_strip")
  endif()

else()
  if(make_library_EXPORT_LINUX_MAP_FILE)
    set_property(TARGET ${NAME} APPEND PROPERTY LINK_DEPENDS "${make_library_EXPORT_LINUX_MAP_FILE}")
    set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--version-script=${make_library_EXPORT_LINUX_MAP_FILE} ")
  endif()
endif()

endmacro()

# Creates an empty library to use as a dependency placeholder.  
# 
# Usage:
#    make_empty_library(NAME)
# 
#
# will automatically include all recursive dependencies.
macro(make_empty_library NAME)
  add_library(${NAME} INTERFACE)
endmacro()

