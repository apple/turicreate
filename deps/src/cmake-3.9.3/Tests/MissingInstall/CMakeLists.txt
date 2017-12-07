cmake_minimum_required (VERSION 2.8.12)
project(TestMissingInstall)

set(CMAKE_SKIP_INSTALL_RULES ON)

# Skip the dependency that causes a build when installing.  This
# avoids infinite loops when the post-build rule below installs.
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY 1)
set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY 1)

if(CMAKE_CONFIGURATION_TYPES)
  set(MULTI_CONFIG ON)
else()
  set(MULTI_CONFIG OFF)
endif()

add_executable(mybin mybin.cpp)
install(TARGETS mybin RUNTIME DESTINATION bin)

add_custom_command(TARGET mybin
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} "-DMULTI_CONFIG=${MULTI_CONFIG}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectInstallFail.cmake
  COMMENT "Install Project"
)
