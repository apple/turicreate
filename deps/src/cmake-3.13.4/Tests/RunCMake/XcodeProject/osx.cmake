set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_VERSION 1)
set(UNIX True)
set(APPLE True)

find_program(XCRUN_EXECUTABLE xcrun)
if(NOT XCRUN_EXECUTABLE)
  message(FATAL_ERROR "xcrun not found")
endif()

execute_process(
  COMMAND ${XCRUN_EXECUTABLE} --sdk macosx --show-sdk-path
  OUTPUT_VARIABLE OSX_SDK_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_OSX_SYSROOT ${OSX_SDK_PATH} CACHE PATH "Sysroot used for OSX support")

set(CMAKE_FIND_ROOT_PATH ${OSX_SDK_PATH} CACHE PATH "Find search path root")
