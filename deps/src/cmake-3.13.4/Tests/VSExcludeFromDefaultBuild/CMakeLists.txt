cmake_minimum_required(VERSION 2.8.9)
project(VSExcludeFromDefaultBuild)

# CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD will enable the INSTALL target to be part of the default build
set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD 1)

# First step is to clear all .exe files in output so that possible past
# failures of this test do not prevent it from succeeding.
add_custom_target(ClearExes ALL
  COMMAND "${CMAKE_COMMAND}"
    -Ddir=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/ClearExes.cmake
  VERBATIM)

# Make sure ClearExes is executed before other targets are built
function(add_test_executable target)
  add_executable("${target}" ${ARGN})
  add_dependencies("${target}" ClearExes)
  install(TARGETS "${target}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/install" OPTIONAL)
endfunction()

add_test_executable(DefaultBuilt main.c)

add_test_executable(AlwaysBuilt main.c)
set_target_properties(AlwaysBuilt PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD FALSE)

add_test_executable(NeverBuilt main.c)
set_target_properties(NeverBuilt PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)

foreach(config ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${config} Config)
  add_test_executable(BuiltIn${config} main.c)
  set_target_properties(BuiltIn${config} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE EXCLUDE_FROM_DEFAULT_BUILD_${Config} FALSE)
  add_test_executable(ExcludedIn${config} main.c)
  set_target_properties(ExcludedIn${config} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD_${Config} TRUE)
endforeach()
