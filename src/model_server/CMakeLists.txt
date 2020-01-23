project(Turi)

subdirs(extensions
  lib)

set(additional_unity_requirements)

if(HAS_CORE_ML)
  set(additional_unity_requirements image_deep_feature_extractor)
endif()

if(NOT TC_DISABLE_OBJECT_BUILDS) 
  set(unity_shared_object_deps
  "$<TARGET_OBJECTS:pylambda_worker>"
  "$<TARGET_OBJECTS:python_callbacks>"
  )
  
if(APPLE AND HAS_COREML_CUSTOM_MODEL)
    set(unity_shared_object_deps
      ${unity_shared_object_deps}
      "$<TARGET_OBJECTS:objcapi>"
      "$<TARGET_OBJECTS:capi>")
endif()

else()
  set(unity_shared_deps 
    pylambda_worker 
    python_callbacks)

  if(APPLE AND HAS_MLCUSTOMMODEL)
    set(unity_shared_deps
      ${unity_shared_deps}
      objcapi
      )
  endif()

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
    perf
    eigen
    ${additional_unity_requirements}
    libbz2
    z
)

# Pull in all the libraries in the extensions directory.
get_property(EXTENSIONS_LIST DIRECTORY 
  "${CMAKE_SOURCE_DIR}/src/model_server/extensions" PROPERTY EXTENSIONS_LIST)

foreach(loop_var ${EXTENSIONS_LIST})
  message(STATUS "unity depending on extension ${loop_var}")
  add_dependencies(unity ${loop_var})
endforeach()

make_library(unity_shared
  SOURCES
    ${unity_shared_object_deps}
    ${_TC_DEFAULT_SERVER_INITIALIZER}
    ${_TC_COMMON_OBJECTS}
  REQUIRES
    ${_TC_COMMON_REQUIREMENTS}
    ${unity_shared_deps}
  SHARED
  SHARED_ALL_DEFINED
  DEAD_STRIP
)

make_library(unity_shared_for_testing
  SOURCES 
    ${unity_shared_objcet_deps}
    ${_TC_DEFAULT_SERVER_INITIALIZER}
    ${_TC_COMMON_OBJECTS}
    $<TARGET_OBJECTS:capi>
  REQUIRES
    ${_TC_COMMON_REQUIREMENTS}
    ${unity_shared_deps}
    libbz2
    visualization
  SHARED 
  SHARED_ALL_DEFINED 
  EXTERNAL_VISIBILITY
)

# Note: this is not actually a build or link time dependency, but both of these
# targets are very expensive to link (in terms of RAM usage).
# Let's make sure they don't link at the same time.
add_dependencies(unity_shared_for_testing unity_shared)

