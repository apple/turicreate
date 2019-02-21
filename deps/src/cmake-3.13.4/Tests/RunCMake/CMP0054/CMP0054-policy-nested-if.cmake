set(FOO BAR)

cmake_policy(SET CMP0054 NEW)

if("FOO" STREQUAL BAR)
  message(FATAL_ERROR "The strings should not match")

  if("FOO" STREQUAL BAR)
    message(FATAL_ERROR "The strings should not match")
  endif()

  cmake_policy(SET CMP0054 OLD)

  if(NOT "FOO" STREQUAL BAR)
    message(FATAL_ERROR "The quoted variable should match the string")
  endif()
endif()

if("FOO" STREQUAL BAR)
  message(FATAL_ERROR "The strings should not match")
endif()

cmake_policy(SET CMP0054 OLD)

if(NOT "FOO" STREQUAL BAR)
  message(FATAL_ERROR "The quoted variable should match the string")

  if(NOT "FOO" STREQUAL BAR)
    message(FATAL_ERROR "The quoted variable should match the string")
  endif()

  cmake_policy(SET CMP0054 NEW)

  if("FOO" STREQUAL BAR)
    message(FATAL_ERROR "The strings should not match")
  endif()
endif()

if(NOT "FOO" STREQUAL BAR)
  message(FATAL_ERROR "The quoted variable should match the string")
endif()
