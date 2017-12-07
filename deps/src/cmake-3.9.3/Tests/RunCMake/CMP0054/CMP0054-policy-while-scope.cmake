set(FOO BAR)

set(LOOP_VAR "")

cmake_policy(SET CMP0054 NEW)

while(NOT LOOP_VAR STREQUAL "xx")
  if(NOT FOO STREQUAL BAR)
    message(FATAL_ERROR "The variable should match the string")
  endif()

  if("FOO" STREQUAL BAR)
    message(FATAL_ERROR "The strings should not match")
  endif()

  cmake_policy(SET CMP0054 OLD)

  if(NOT FOO STREQUAL BAR)
    message(FATAL_ERROR "The variable should match the string")
  endif()

  if(NOT "FOO" STREQUAL BAR)
    message(FATAL_ERROR "The quoted variable should match the string")
  endif()

  cmake_policy(SET CMP0054 NEW)

  string(APPEND LOOP_VAR "x")
endwhile()

while("FOO" STREQUAL BAR)
  message(FATAL_ERROR "The strings should not match")
endwhile()

set(LOOP_VAR "")

cmake_policy(SET CMP0054 OLD)

while(NOT LOOP_VAR STREQUAL "xx")
  if(NOT FOO STREQUAL BAR)
    message(FATAL_ERROR "The variable should match the string")
  endif()

  if(NOT "FOO" STREQUAL BAR)
    message(FATAL_ERROR "The quoted variable should match the string")
  endif()

  cmake_policy(SET CMP0054 NEW)

  if(NOT FOO STREQUAL BAR)
    message(FATAL_ERROR "The variable should match the string")
  endif()

  if("FOO" STREQUAL BAR)
    message(FATAL_ERROR "The strings should not match")
  endif()

  cmake_policy(SET CMP0054 OLD)

  string(APPEND LOOP_VAR "x")
endwhile()

if(NOT "FOO" STREQUAL BAR)
  message(FATAL_ERROR "The quoted variable should match the string")
endif()
