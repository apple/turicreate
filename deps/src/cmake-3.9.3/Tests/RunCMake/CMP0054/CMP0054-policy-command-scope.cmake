set(FOO BAR)

cmake_policy(SET CMP0054 NEW)

function(function_defined_new_called_old)
  if(NOT FOO STREQUAL BAR)
    message(FATAL_ERROR "The variable should match the string")
  endif()

  if("FOO" STREQUAL BAR)
    message(FATAL_ERROR "The strings should not match")
  endif()
endfunction()

macro(macro_defined_new_called_old)
  if(NOT FOO STREQUAL BAR)
    message(FATAL_ERROR "The variable should match the string")
  endif()

  if("FOO" STREQUAL BAR)
    message(FATAL_ERROR "The strings should not match")
  endif()
endmacro()

cmake_policy(SET CMP0054 OLD)

function_defined_new_called_old()
macro_defined_new_called_old()

function(function_defined_old_called_new)
  if(NOT FOO STREQUAL BAR)
    message(FATAL_ERROR "The variable should match the string")
  endif()

  if(NOT "FOO" STREQUAL BAR)
    message(FATAL_ERROR "The quoted variable should match the string")
  endif()
endfunction()

macro(macro_defined_old_called_new)
  if(NOT FOO STREQUAL BAR)
    message(FATAL_ERROR "The variable should match the string")
  endif()

  if(NOT "FOO" STREQUAL BAR)
    message(FATAL_ERROR "The quoted variable should match the string")
  endif()
endmacro()

cmake_policy(SET CMP0054 NEW)

function_defined_old_called_new()
macro_defined_old_called_new()
