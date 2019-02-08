project(unity)

subdirs(extensions
  lib
  toolkits)

  add_library(unity OBJECT
    server/registration.cpp
    server/unity_server.cpp
    server/unity_server_init.cpp
    server/unity_server_control.cpp
    server/unity_server_init.cpp
  )
  add_dependencies(unity boost)

  # Pull in all the libraries in the extensions directory.
  get_property(EXTENSIONS_LIST DIRECTORY "extensions" PROPERTY EXTENSIONS_LIST)

  foreach(loop_var ${EXTENSIONS_LIST})
    message(STATUS "unity depending on extension ${loop_var}")
    add_dependencies(unity ${loop_var})
  endforeach()

  set(unity_shared_deps
    "$<TARGET_OBJECTS:pylambda_worker>"
    "$<TARGET_OBJECTS:python_callbacks>"
  )

  if(APPLE AND HAS_MLCUSTOMMODEL)
    set(unity_shared_deps
      ${unity_shared_deps}
      "$<TARGET_OBJECTS:objcapi>"
    )
  endif()

  if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp "")
  endif()
  make_library(unity_shared
    SOURCES
      ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp
      ${unity_shared_deps}
    REQUIRES
      TuriCreate_static
    EXPORT_LINUX_MAP_FILE "${CMAKE_SOURCE_DIR}/cmake/stdcxx_reduce_exports.ver"
    # TODO this doesn't work on macOS, but doesn't appear to need to?
    # (i.e. the collect_archive.sh script wasn't applying this on macOS previously)
    #EXPORT_OSX_MAP_FILE "${CMAKE_SOURCE_DIR}/cmake/stdcxx_reduce_exports.ver"
    SHARED
    SHARED_ALL_DEFINED
    DEAD_STRIP
  )

  # same as unity_shared, but all symbols exported
  # uses add_library instead of make_library so dependencies' symbols can be exported.
  add_library(unity_shared_for_testing SHARED
    ${unity_shared_deps}
  )
  target_link_libraries(unity_shared_for_testing PUBLIC TuriCreate_static)

  # TODO - will this bloat the .so?
  # If so, how do we get _just_ the right set of symbols exposed?
  # ... how did we before this change??
  if(CLANG)
    set_target_properties(unity_shared PROPERTIES LINK_FLAGS "-Wl,-all_load")
    set_target_properties(unity_shared_for_testing PROPERTIES LINK_FLAGS "-Wl,-all_load")
  endif()

# TODO are these used anywhere?
set(UNITY_ALL_LIBRARIES unity_shared)
set(UNITY_EXTENSION_LIBRARY unity_shared)

if(${TC_BUILD_PYTHON})
  subdirs(python)

  include_directories(${PYTHON_INCLUDE_DIRS})
  message("Python library = ${PYTHON_LIBRARIES}")

endif()
