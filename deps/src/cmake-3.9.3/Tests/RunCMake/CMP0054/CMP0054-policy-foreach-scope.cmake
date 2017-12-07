set(FOO BAR)

cmake_policy(SET CMP0054 NEW)

foreach(loop_var arg1 arg2)
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
endforeach()

cmake_policy(SET CMP0054 OLD)

foreach(loop_var arg1 arg2)
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
endforeach()
