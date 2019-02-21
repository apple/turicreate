cmake_minimum_required (VERSION 3.9)
project(TestMissingInstall)

set(CMAKE_SKIP_INSTALL_RULES ON)

# Skip the dependency that causes a build when installing.  This
# avoids infinite loops when the post-build rule below installs.
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY 1)
set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY 1)

get_property(MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

add_executable(mybin mybin.cpp)
install(TARGETS mybin RUNTIME DESTINATION bin)

add_custom_command(TARGET mybin
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} "-DMULTI_CONFIG=${MULTI_CONFIG}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectInstallFail.cmake
  COMMENT "Install Project"
)
