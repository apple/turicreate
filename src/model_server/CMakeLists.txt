project(unity)

subdirs(extensions
  lib)

set(additional_unity_requirements)
if(HAS_CORE_ML)
  set(additional_unity_requirements image_deep_feature_extractor)
endif()

make_library(unity OBJECT
  SOURCES
    server/registration.cpp
    server/unity_server.cpp
    server/unity_server_init.cpp
    server/unity_server_control.cpp
    server/unity_server_init.cpp
  REQUIRES
    unity_core
    unity_toolkits
    perf
    eigen
    ${additional_unity_requirements}
)

# Pull in all the libraries in the extensions directory.
get_property(EXTENSIONS_LIST DIRECTORY "${CMAKE_SOURCE_DIR}/src/model_server/extensions" PROPERTY EXTENSIONS_LIST)

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

make_library(unity_shared
  SOURCES
    ${unity_shared_deps}
    ${_TC_DEFAULT_SERVER_INITIALIZER}
    ${_TC_COMMON_OBJECTS}
  REQUIRES
    ${_TC_COMMON_REQUIREMENTS}
  SHARED
  SHARED_ALL_DEFINED
  DEAD_STRIP
)

# same as unity_shared, but all symbols exported
# uses add_library instead of make_library so dependencies' symbols can be exported.
add_library(unity_shared_for_testing SHARED
  ${unity_shared_deps}
  ${_TC_DEFAULT_SERVER_INITIALIZER}
  ${_TC_COMMON_OBJECTS}
)
target_link_libraries(unity_shared_for_testing ${_TC_COMMON_REQUIREMENTS})

# Note: this is not actually a build or link time dependency, but both of these
# targets are very expensive to link (in terms of RAM usage).
# Let's make sure they don't link at the same time.
add_dependencies(unity_shared_for_testing unity_shared)

