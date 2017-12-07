if(NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()
include(ExternalProject)

ExternalProject_Add(FOO TMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/tmp"
                        DOWNLOAD_COMMAND ""
                        CMAKE_CACHE_ARGS "-DFOO:STRING=$<1:BAR>$<0:BAD>"
                        CMAKE_CACHE_DEFAULT_ARGS "-DBAR:STRING=$<1:BAZ>$<0:BAD>")
