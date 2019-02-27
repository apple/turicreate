cmake_minimum_required(VERSION 3.10)
project(CleanByproducts)

# Configurable parameters
set(TEST_CLEAN_NO_CUSTOM FALSE CACHE BOOL "Value for the CLEAN_NO_CUSTOM PROPERTY")
set(TEST_BUILD_EVENTS TRUE CACHE BOOL "Create byproducts with build events")
set(TEST_CUSTOM_TARGET TRUE CACHE BOOL "Create a byproduct with a custom target")
set(TEST_CUSTOM_COMMAND TRUE CACHE BOOL "Create a byproduct with a custom command")

set_property(DIRECTORY PROPERTY CLEAN_NO_CUSTOM ${TEST_CLEAN_NO_CUSTOM})

macro(add_build_event)
  set(oneValueArgs EVENT)

  cmake_parse_Arguments(ABE "" "${oneValueArgs}" "" ${ARGN})

  # Create two byproducts and only declare one
  add_custom_command(TARGET foo
    ${ABE_EVENT}
    COMMAND ${CMAKE_COMMAND} -E touch foo.${ABE_EVENT}
    COMMAND ${CMAKE_COMMAND} -E touch foo.${ABE_EVENT}.notdeclared
    COMMENT "Creating byproducts with ${ABE_EVENT}"
    BYPRODUCTS foo.${ABE_EVENT}
  )

  # The nondeclared byproduct should always be present
  list(APPEND EXPECTED_PRESENT foo.${ABE_EVENT}.notdeclared)

  # If CLEAN_NO_CUSTOM is set, the declared byproduct should be present
  if(TEST_CLEAN_NO_CUSTOM)
    list(APPEND EXPECTED_PRESENT foo.${ABE_EVENT})
  else()
    list(APPEND EXPECTED_DELETED foo.${ABE_EVENT})
  endif()
endmacro()

add_executable(foo foo.cpp)

# Test build events
if(TEST_BUILD_EVENTS)
  add_build_event(EVENT "PRE_BUILD" ENABLE ${TEST_PRE_BUILD})
  add_build_event(EVENT "PRE_LINK" ENABLE ${TEST_PRE_LINK})
  add_build_event(EVENT "POST_BUILD" ENABLE ${TEST_POST_BUILD})
endif()

# Custom command that generates byproducts
if(TEST_CUSTOM_COMMAND)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bar.cpp.in "void bar() {}\n")
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bar.cpp
    COMMAND ${CMAKE_COMMAND} -E touch foo.customcommand
    COMMAND ${CMAKE_COMMAND} -E touch foo.customcommand.notdeclared
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/bar.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/bar.cpp
    BYPRODUCTS foo.customcommand
    COMMENT "Creating byproducts with a custom command"
  )

  # The nondeclared byproduct should always be present
  list(APPEND EXPECTED_PRESENT "foo.customcommand.notdeclared")

  # If CLEAN_NO_CUSTOM is set, both the output and byproduct should be present
  if(TEST_CLEAN_NO_CUSTOM)
    list(APPEND EXPECTED_PRESENT "bar.cpp")
    list(APPEND EXPECTED_PRESENT "foo.customcommand")
  else()
    list(APPEND EXPECTED_DELETED "bar.cpp")
    list(APPEND EXPECTED_DELETED "foo.customcommand")
  endif()

  target_sources(foo PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/bar.cpp")
endif()

# Custom target that generates byproducts
if(TEST_CUSTOM_TARGET)
  add_custom_target(foo_file ALL
    DEPENDS foo
    COMMAND ${CMAKE_COMMAND} -E touch foo.customtarget
    COMMAND ${CMAKE_COMMAND} -E touch foo.customtarget.notdeclared
    BYPRODUCTS foo.customtarget
    COMMENT "Creating byproducts with a custom target"
  )

  # The nondeclared byproduct should always be present
  list(APPEND EXPECTED_PRESENT "foo.customtarget.notdeclared")

  # If CLEAN_NO_CUSTOM is set, the declared byproduct should be present
  if(TEST_CLEAN_NO_CUSTOM)
    list(APPEND EXPECTED_PRESENT "foo.customtarget")
  else()
    list(APPEND EXPECTED_DELETED "foo.customtarget")
  endif()
endif()

configure_file(files.cmake.in files.cmake)
