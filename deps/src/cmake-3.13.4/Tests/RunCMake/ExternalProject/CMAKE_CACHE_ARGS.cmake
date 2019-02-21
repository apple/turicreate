get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _isMultiConfig)
  set(CMAKE_BUILD_TYPE Debug)
endif()
include(ExternalProject)

ExternalProject_Add(FOO TMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/tmp"
                        DOWNLOAD_COMMAND ""
                        CMAKE_CACHE_ARGS
                            "-DFOO:STRING=$<1:BAR>$<0:BAD>"
                            "-DTEST_LIST:STRING=A;B;C"
                            "-DTEST_OPTIONAL:FILEPATH=TEST_OPTIONAL-NOTFOUND"
                            "-DTEST_OPTIONAL_LAST:FILEPATH=TEST_OPTIONAL_LAST-NOTFOUND")
