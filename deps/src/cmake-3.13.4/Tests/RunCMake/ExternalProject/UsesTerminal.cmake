get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _isMultiConfig)
  set(CMAKE_BUILD_TYPE Debug)
endif()
include(ExternalProject)

# Test various combinations of USES_TERMINAL with ExternalProject_Add.

macro(DoTerminalTest _target)
  ExternalProject_Add(${_target}
    DOWNLOAD_COMMAND "${CMAKE_COMMAND}" -E echo "download"
    UPDATE_COMMAND "${CMAKE_COMMAND}" -E echo "update"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "configure"
    BUILD_COMMAND "${CMAKE_COMMAND}" -E echo "build"
    TEST_COMMAND "${CMAKE_COMMAND}" -E echo "test"
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo "install"
    ${ARGN}
    )
endmacro()

# USES_TERMINAL on all steps
DoTerminalTest(TerminalTest1
  USES_TERMINAL_DOWNLOAD 1
  USES_TERMINAL_UPDATE 1
  USES_TERMINAL_CONFIGURE 1
  USES_TERMINAL_BUILD 1
  USES_TERMINAL_TEST 1
  USES_TERMINAL_INSTALL 1
  )

# USES_TERMINAL on every other step, starting with download
DoTerminalTest(TerminalTest2
  USES_TERMINAL_DOWNLOAD 1
  USES_TERMINAL_CONFIGURE 1
  USES_TERMINAL_TEST 1
  )

# USES_TERMINAL on every other step, starting with update
DoTerminalTest(TerminalTest3
  USES_TERMINAL_UPDATE 1
  USES_TERMINAL_BUILD 1
  USES_TERMINAL_INSTALL 1
  )

# USES_TERMINAL on no step
DoTerminalTest(TerminalTest4)
