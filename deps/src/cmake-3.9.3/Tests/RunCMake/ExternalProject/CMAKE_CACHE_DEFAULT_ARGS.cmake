if(NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()
include(ExternalProject)

ExternalProject_Add(FOO TMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/tmp"
                        DOWNLOAD_COMMAND ""
                        CMAKE_CACHE_DEFAULT_ARGS
                            "-DFOO:STRING=$<1:BAR>$<0:BAD>"
                            "-DTEST_LIST:STRING=A;B;C")
