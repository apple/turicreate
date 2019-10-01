# Disable warnings to avoid changing 3rd party code.
IF(CMAKE_C_COMPILER_ID MATCHES
    "^(GNU|Clang|AppleClang|XL|VisualAge|SunPro|MIPSpro|HP|Intel)$")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
ELSEIF(CMAKE_C_COMPILER_ID STREQUAL "PathScale")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -woffall")
ENDIF()

include(ConfigureChecks.cmake)
if(NOT WIN32)
  add_definitions(-DXML_DEV_URANDOM)
endif()

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/lib
  )

add_library(cmexpat STATIC
  lib/loadlibrary.c
  lib/xmlparse.c
  lib/xmlrole.c
  lib/xmltok.c
  lib/xmltok_impl.c
  lib/xmltok_ns.c
  )
