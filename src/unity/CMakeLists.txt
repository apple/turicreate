project(unity)

subdirs(extensions
  lib
  toolkits)

if(HAS_CORE_ML)
  set(additional_unity_requirements image_deep_feature_extractor)
else()
  set(additional_unity_requirements "")
endif()

  make_library(unity
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
          numerics
          ${additional_unity_requirements}
         COMPILE_FLAGS_EXTRA_GCC
          -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
          -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  ) 

  # Pull in all the libraries in the extensions directory.
  get_property(EXTENSIONS_LIST DIRECTORY "extensions" PROPERTY EXTENSIONS_LIST)

  foreach(loop_var ${EXTENSIONS_LIST})
    message(STATUS "unity depending on extension ${loop_var}")
    add_dependencies(unity ${loop_var})
  endforeach()


if(${TC_BUILD_PYTHON})
  subdirs(python)

  include_directories(${PYTHON_INCLUDE_DIRS})
  message("Python library = ${PYTHON_LIBRARIES}")

  make_empty_library(unity_python REQUIRES unity_shared)
endif()
